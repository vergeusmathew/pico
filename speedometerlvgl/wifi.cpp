#include "wifi.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "ds3231.hpp"

#define UART_ID uart1
#define BUFFER_SIZE 512
static char line_buffer[BUFFER_SIZE];

extern Ds3231 rtc;

#define WIFI_SPINLOCK_ID 31

volatile bool g_wifi_hotspot_available = false;
volatile bool g_rtc_sync_successful = false;
volatile bool g_wifi_scanning_complete = false;

volatile int g_parsed_year = 0;
volatile int g_parsed_month = 0;
volatile int g_parsed_day = 0;
volatile int g_parsed_hour = 0;
volatile int g_parsed_minute = 0;
volatile int g_parsed_second = 0;
volatile bool g_parsed_time_ready_to_load = false;

// Empties any remaining raw data inside the Pico's serial registers
static void flush_uart() {
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }
}

// Sends an AT command and blocks until the expected confirmation or timeout occurs
static bool send_at_command_blocking(const char* cmd, const char* expected_resp, uint32_t timeout_ms) {
    flush_uart(); 
    printf("\n[WIFI TX] Sending: %s", cmd);
    uart_puts(UART_ID, cmd);

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    int idx = 0;
    memset(line_buffer, 0, BUFFER_SIZE);

    while ((to_ms_since_boot(get_absolute_time()) - start_time) < timeout_ms) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            
            if (idx < (BUFFER_SIZE - 1)) {
                line_buffer[idx++] = c;
                line_buffer[idx] = '\0';
            }

            if (strstr(line_buffer, expected_resp) != NULL) {
                printf("\n---> [SUCCESS] Found expected response: \"%s\"\n", expected_resp);
                return true;
            }
            if (strstr(line_buffer, "ERROR") != NULL || strstr(line_buffer, "FAIL") != NULL) {
                printf("\n---> [FAILURE] Module rejected command with ERROR/FAIL flag.\n");
                return false;
            }
        }
    }
    printf("\n---> [TIMEOUT] Failed to receive \"%s\" within %d ms.\n", expected_resp, timeout_ms);
    return false;
}

// LIVE HOTSPOT NETWORK AVAILABILITY SCANNER 
bool IsMobileHotspotAvailable(const char* target_ssid) {
    printf("[WIFI CHECK] Verifying link registration state for SSID: %s...\n\r", target_ssid);
    
    flush_uart();
    memset(line_buffer, 0, BUFFER_SIZE);
    
    // Ensure Station client profile mode is locked in place cleanly
    send_at_command_blocking("AT+CWMODE=1\r\n", "OK", 1000);
    sleep_ms(200);
    flush_uart();

    // Query the ESP-01 firmware directly for active connection details
    uart_puts(UART_ID, "AT+CWJAP_CUR?\r\n");
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    int idx = 0;
    memset(line_buffer, 0, BUFFER_SIZE);
    
    // Fast 1.5-second listen window to verify existing link parameters
    while ((to_ms_since_boot(get_absolute_time()) - start_time) < 1500) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (idx < (BUFFER_SIZE - 1)) {
                line_buffer[idx++] = c;
                line_buffer[idx] = '\0';
            }
            
            // If the query returns your phone's name string, the link is live!
            if (strstr(line_buffer, target_ssid) != NULL) {
                printf("[WIFI CHECK] Target hotspot connection confirmed active and verified!\n\r");
                
                uint32_t irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
                g_wifi_hotspot_available = true;
                spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
                
                return true;
            }
        }
    }
    
    printf("[WIFI CHECK] Target SSID not currently linked to module.\n\r");
    
    uint32_t irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
    g_wifi_hotspot_available = false;
    spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
    
    return false;
}

void SyncRtcWithInternetTime() {
	g_rtc_sync_successful = false;
    printf("\n[SYSTEM] Triggering Internet Synchronization Module via ESP-01...\n\r");
    
    // Clear out any old hardware register data remnants before processing
    flush_uart();

    // STEP 1: Test basic heartbeat ping connection
    if (!send_at_command_blocking("AT\r\n", "OK", 1000)) {
        printf("[WIFI ERROR] ESP-01 not answering AT pings. Sync aborted.\n\r");
        return;
    }
    
    // STEP 2: Configure MUX state machine mode to Single Connection
    if (!send_at_command_blocking("AT+CIPMUX=0\r\n", "OK", 1000)) {
        printf("[WIFI ERROR] Failed to configure multiplexer settings. Sync aborted.\n\r");
        return;
    }
    
    // STEP 3: Connect to the NIST Daytime open network text string stream socket
    if (!send_at_command_blocking("AT+CIPSTART=\"TCP\",\"time.nist.gov\",13\r\n", "CONNECT", 4000)) {
        printf("[WIFI ERROR] Could not open connection socket to NIST time server.\n\r");
        return;
    }

    // STEP 4: Capture the streaming data payload block safely into line_buffer
    printf("[WIFI RX] Socket opened! Collecting streaming payload text lines...\n\r");
    memset(line_buffer, 0, BUFFER_SIZE);
    int idx = 0;
    uint32_t stream_start = to_ms_since_boot(get_absolute_time());
    
    while ((to_ms_since_boot(get_absolute_time()) - stream_start) < 2500) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (idx < (BUFFER_SIZE - 1)) {
                line_buffer[idx++] = c;
                line_buffer[idx] = '\0';
            }
        }
    }
    
    printf("\n--- Stream Collection Complete. Parsing Data ---\n\r");

    // Scan line_buffer for the explicit NIST date dashes pattern matching: YY-MM-DD
    char *date_ptr = NULL;
    for (int i = 0; i < idx - 8; i++) {
        if (line_buffer[i] == '-' && line_buffer[i+3] == '-') {
            date_ptr = &line_buffer[i-2]; 
            break;
        }
    }

    if (date_ptr != NULL) {
        int year, month, day, hour, minute, second;
        
        if (sscanf(date_ptr, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
            
            // --- TIMEZONE CONVERSION FOR INDIA (IST = UTC + 5:30) ---
            int ist_hour = hour + 5;
            int ist_minute = minute + 30;
            int ist_day = day;
			int full_year = year + 2000;

            if (ist_minute >= 60) {
                ist_minute -= 60;
                ist_hour += 1;
            }
            if (ist_hour >= 24) {
                ist_hour -= 24;
                ist_day += 1;
            }

            printf("[NTP SYNC] Extracted Time: %02d:%02d:%02d | Extracted Date: %02d/%02d/20%02d\n\r", 
                   ist_hour, ist_minute, second, ist_day, month, year);
            
           // Call your distinct hardware methods back-to-back cleanly
            rtc.SetTime((uint8_t)ist_hour, (uint8_t)ist_minute, (uint8_t)second); // Writes registers 0x00-0x02
            rtc.SetDate((uint8_t)ist_day,  (uint8_t)month,      (uint16_t)full_year); // Writes registers 0x04-0x06
            g_rtc_sync_successful = true; 
            printf("[NTP SYNC] Success! Your class calibrated the hardware chip cleanly.\n\r");
                   
        } else {
            printf("[WIFI ERROR] Found pattern position, but text values failed parsing logic.\n\r");
        }
    } else {
        printf("[WIFI ERROR] Structural date dashes (-.-) could not be located inside tracking buffer context.\n\r");
    }
}

void Core1_Wifi_Worker_Thread() {
    // Initial 1-second resting delay to let the ESP-01 power rail stabilize fully
    sleep_ms(1000); 
    printf("[CORE 1] Asynchronous Wi-Fi Network thread active.\n\r");

    RtcTime boot_check_time = rtc.GetTime();
    
    // Double-check power loss state tracking condition variables
    if (boot_check_time.year == 2000 || boot_check_time.year == 0) {
        
        // 🌟 STEP A: POLL AT+CIPSTATUS EVERY 500MS FOR 6 SECONDS
        // This naturally catches the module the exact microsecond it auto-connects!
        printf("[CORE 1] Monitoring UART lines for native auto-connection...\n\r");
        bool network_ready = false;
        uint32_t polling_start = to_ms_since_boot(get_absolute_time());
        
        while ((to_ms_since_boot(get_absolute_time()) - polling_start) < 6000) {
            flush_uart();
            // STATUS:2 confirms the module successfully holds a valid IP address
            if (send_at_command_blocking("AT+CIPSTATUS\r\n", "STATUS:2", 400)) {
                printf("[CORE 1] Native Auto-Connection caught successfully! Skipping manual link.\n\r");
                network_ready = true;
                break;
            }
            sleep_ms(500); 
        }

        // 🌟 STEP B: FALLBACK - IF NOT AUTO-CONNECTED, FORCE A BLIND JOIN DIRECTLY
        // If auto-connect didn't fire, connect directly via AT+CWJAP without scanning!
        if (!network_ready) {
            printf("[CORE 1] Hotspot not auto-linked. Checking visibility fallback...\n\r");
            
            if (IsMobileHotspotAvailable("Redmi A4 5G")) {
                printf("[CORE 1] Settle delay window opening...\n\r");
                sleep_ms(1000); 
                flush_uart(); 
                
                printf("[CORE 1] Transmitting manual connection handshake tokens...\n\r");
                // Wait securely for "WIFI GOT IP" to guarantee the radio link is established
                if (send_at_command_blocking("AT+CWJAP=\"Redmi A4 5G\",\"SHoJIHoM@3773\"\r\n", "WIFI GOT IP", 25000)) {
                    printf("[CORE 1] Manual network join successful!\n\r");
                    network_ready = true;
                } else {
                    printf("[CORE 1] Manual network join timed out or failed.\n\r");
                }
            }
        }

        // Secure shared hotspot status visibility flag update behind the gate
        uint32_t irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
        g_wifi_hotspot_available = network_ready; 
        spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);

        // 🌟 STEP C: OPEN SOCKET CHANNELS AND STREAM TIME METRICS
        if (network_ready) {
            sleep_ms(1000); 
            flush_uart();
            printf("[CORE 1] Opening secure NIST time server socket channels...\n\r");
            
            if (send_at_command_blocking("AT+CIPSTART=\"TCP\",\"time.nist.gov\",13\r\n", "CONNECT", 5000)) {
                
                memset(line_buffer, 0, BUFFER_SIZE);
                int idx = 0;
                uint32_t stream_start = to_ms_since_boot(get_absolute_time());
                
                while ((to_ms_since_boot(get_absolute_time()) - stream_start) < 2500) {
                    if (uart_is_readable(UART_ID)) {
                        char c = uart_getc(UART_ID);
                        if (idx < (BUFFER_SIZE - 1)) {
                            line_buffer[idx++] = c;
                            line_buffer[idx] = '\0';
                        }
                    }
                }

                char *date_ptr = NULL;
                for (int i = 0; i < idx - 8; i++) {
                    if (line_buffer[i] == '-' && line_buffer[i+3] == '-') {
                        date_ptr = &line_buffer[i-2]; 
                        break;
                    }
                }

                if (date_ptr != NULL) {
                    int year, month, day, hour, minute, second;
                    if (sscanf(date_ptr, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
                        int ist_hour = hour + 5;
                        int ist_minute = minute + 30;
                        int ist_day = day;
                        int full_year = year + 2000;

                        if (ist_minute >= 60) { ist_minute -= 60; ist_hour += 1; }
                        if (ist_hour >= 24) { ist_hour -= 24; ist_day += 1; }

                        // Secure inter-core RAM bridge buffer data transfer passing
                        irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
                        g_parsed_hour   = ist_hour;
                        g_parsed_minute = ist_minute;
                        g_parsed_second = second;
                        g_parsed_day    = ist_day;
                        g_parsed_month  = month;
                        g_parsed_year   = full_year;
                        g_parsed_time_ready_to_load = true; 
                        g_rtc_sync_successful = true; 
                        spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
                        
                        printf("[CORE 1] Network data safely written behind spinlock gate.\n\r");
                    }
                }
            }
        } else {
            irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
            g_rtc_sync_successful = false;
            spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
        }
    } else {
        // Battery backup time is valid! Flag indicators healthy instantly
        uint32_t irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
        g_rtc_sync_successful = true;
        g_wifi_hotspot_available = true;
        spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
    }

    // 🌟 GATE LOCK UP: Secure the final scanning complete execution checkpoint
    uint32_t final_irq = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
    g_wifi_scanning_complete = true;
    spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), final_irq);

    printf("[CORE 1] Wi-Fi tasks concluded cleanly. Thread exiting naturally.\n\r");
}
