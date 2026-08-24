//git clone https://github.com/lvgl/lvgl
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h" 	// REQUIRED FOR DUAL CORE EXECUTION
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/regs/resets.h"
#include <sys/time.h>
#include "Gen1Hz.pio.h"
#include "lcd.hpp"
#include "lv_image_assets.hpp"
#include "ds3231.hpp"
//#include "eeprom.hpp"
#include "lvgl.h"
#include "gui.hpp"
#include "wifi.hpp"


#define UART_ID uart1
#define BAUD_RATE 115200

// Map pins to GP4 and GP5 instead of GP0 and GP1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Define Button Input pins matching your physical navigation layout
#define KEY3_PIN 11  // ENTER / CLICK (Top Button)
#define KEY2_PIN 12  // NAVIGATE / MOVE RING (Middle Button)
#define KEY1_PIN 13  // ESCAPE / BACK (Bottom Button)

// Shared non-blocking button state triggers updated by hardware ISR lines
volatile bool key1_pressed = false;
volatile bool key2_pressed = false;
volatile bool key3_pressed = false;

extern volatile bool g_wifi_hotspot_available;
extern volatile bool g_rtc_sync_successful;
extern volatile bool g_wifi_scanning_complete;

bool g_core1_is_active = false;

void my_keypad_read_cb(lv_indev_t * indev, lv_indev_data_t * data);
// Forward declaration of hardware callbacks
extern "C" void shared_button_callback(uint gpio, uint32_t events);

extern "C" {
    uint Generate1HzOnLED(PIO pio, int gpio);
}

// Wrapper routing callback to help LVGL tie securely into your C++ display object class instance
static void lvgl_flush_wrapper(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    lcd.FlushLvglDisplay(disp, area, px_map); // Fires high-speed DMA pipeline
}

//export PICO_SDK_PATH=../../pico-sdk/
//rm -rf *
//cmake ..
//make -j$(nproc)
//df -h | grep RPI-RP2
//sudo cp myTFT_SPI_DMA.uf2 /media/mathew/RPI-RP2/
//minicom -b 115200 -o -D /dev/ttyACM0
int main() {
	
	//const uint CONSOLE_BAUD_RATE = 460800; 
    //uart_init(uart0, CONSOLE_BAUD_RATE);
    stdio_init_all();
    sleep_ms(2000); // Give your computer terminal a moment to mount the virtual Minicom USB port
    
    printf("PTLA Aug18,2026 14:24...\n");
    
    const uint LED_DEBUG_PIN = 6;
	gpio_init(LED_DEBUG_PIN);
	gpio_set_dir(LED_DEBUG_PIN, GPIO_OUT);
	gpio_put(LED_DEBUG_PIN, 1); // Start with the LED off
	
	// Initialize UART1 Hardware
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    lcd.Inithw(); // Activates ILI9341 display register commands pipelines
    rtc.Init();
   
    Generate1HzOnLED(pio1, PICO_DEFAULT_LED_PIN);
    
    RtcTime boot_check_time = rtc.GetTime();
    bool should_launch_core1 = false;
    
    if (boot_check_time.year == 2000 || boot_check_time.year == 0) {
        printf("[BOOT] RTC Reset. Spinning off Wi-Fi Sync to Core 1...\n\r");
        
        g_wifi_scanning_complete = false; // Tells the UI to display yellow [SCAN]
        g_core1_is_active = true; 
        should_launch_core1 = true; 
        //multicore_launch_core1(Core1_Wifi_Worker_Thread);
    } else {
        printf("[BOOT] Valid time. Skipping network tasks entirely.\n\r");
        
        // 🚀 Set your healthy status markers instantly before the UI loads!
        g_rtc_sync_successful = true;
        g_wifi_hotspot_available = true;
        g_wifi_scanning_complete = true; // Tells the UI to skip scanning and display [OK] immediately
        
        g_core1_is_active = false; //  Keep false because Core 1 is dormant
    }
    
    // ------------------------------------------------------------------------
    // 🌟 INITIALIZE THE NATIVE LVGL ENGINE SUBSYSTEM
    // ------------------------------------------------------------------------
    lv_init();

    // Allocate an internal graphic display driver allocation context block
    lv_display_t * disp = lv_display_create(320, 240);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    // Link LVGL directly to your optimized hardware flush driver method wrapper
    lv_display_set_flush_cb(disp, lvgl_flush_wrapper);

    // Create a rendering draw buffer memory slot pool
    // 1/10th of screen size (320x24 pixels) provides smooth updates without exhausting your Pico RAM
    //static uint8_t buf1[320 * 24 * 2]; 
    //lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	static uint8_t buf1[320 * 60 * 2]; 
    
    // Configure the engine to run in full-screen drawing mode
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    // ------------------------------------------------------------------------
    // 🌟 REGISTER THE BUTTON READ MECHANICS INTO THE ENGINE GROUP
    // ------------------------------------------------------------------------
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD); //LV_INDEV_TYPE_ENCODER); // Configures keypad/encoder mode navigation
    lv_indev_set_read_cb(indev, my_keypad_read_cb);

    // Setup input button pins with your 4k7 external pull-up configurations
    gpio_init(KEY1_PIN); gpio_set_dir(KEY1_PIN, GPIO_IN); gpio_disable_pulls(KEY1_PIN);
    gpio_init(KEY2_PIN); gpio_set_dir(KEY2_PIN, GPIO_IN); gpio_disable_pulls(KEY2_PIN);
    gpio_init(KEY3_PIN); gpio_set_dir(KEY3_PIN, GPIO_IN); gpio_disable_pulls(KEY3_PIN);

    // Attach pins into our plain C linkage shared interrupt pipeline handles
    gpio_set_irq_enabled_with_callback(KEY1_PIN, GPIO_IRQ_EDGE_FALL, true, &shared_button_callback);
    gpio_set_irq_enabled(KEY2_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(KEY3_PIN, GPIO_IRQ_EDGE_FALL, true);

    printf("LVGL Interface Core Operational. Initializing UI Layout Scenes...\n");
    
    // TODO: We will write this layout creator function block next to render your actual icons!
    create_lvgl_home_screen();

	if (should_launch_core1) {
        printf("[BOOT] Launching Core 1 Thread Safely Now...\n\r");
        multicore_launch_core1(Core1_Wifi_Worker_Thread);
    }
    // ------------------------------------------------------------------------
    // 🌟 THE BACKGROUND PROCESSOR AND TASK LOOP TICK ENGINE
    // ------------------------------------------------------------------------
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t last_tick = current_time;

    while (true) {
        current_time = to_ms_since_boot(get_absolute_time());
        // Ensure at least 1 millisecond has actually elapsed before updating the engine clock
        if (current_time > last_tick) {
            uint32_t elapsed = current_time - last_tick;
            lv_tick_inc(elapsed); // Push elapsed ms into the internal LVGL timer wheels
            last_tick = current_time;
        }

        // Periodically executes active layout tasks, refreshes canvas buffers, and checks buttons
        lv_timer_handler(); 
        
        sleep_ms(10); // Paces thread execution constraints to preserve board lifecycle thermal rails
    }
}

// Interrupt Service Routine - Fast flag tripping exit logic to shield scheduling states
extern "C" void shared_button_callback(uint gpio, uint32_t events) {
    if (gpio == KEY3_PIN) {
        gpio_set_irq_enabled(KEY3_PIN, GPIO_IRQ_EDGE_FALL, false);
        key3_pressed = true;
    }
    else if (gpio == KEY2_PIN) {
        gpio_set_irq_enabled(KEY2_PIN, GPIO_IRQ_EDGE_FALL, false);
        key2_pressed = true;
    }
    else if (gpio == KEY1_PIN) {
        gpio_set_irq_enabled(KEY1_PIN, GPIO_IRQ_EDGE_FALL, false);
        key1_pressed = true;
    }
}

uint32_t g_last_physical_key = 0;
bool g_request_indev_reset = false;

// Ensure this global variable matches your top-level definition declaration fields
extern uint32_t g_last_physical_key;

void my_keypad_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    static uint32_t last_key = 0;
    static bool key_is_locked = false; 
    const uint LED_DEBUG_PIN = 6; 
    
    // If a screen transition occurred, force-clear the driver back to an official 
    // RELEASED resting state on this cycle to unblock the input manager queues!
    if (g_request_indev_reset) {
		printf("[DB_PAD] 1");
        g_request_indev_reset = false;
        key_is_locked = false;
        last_key = 0;
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = 0;
        return;
    }
    
    // 1. Direct hardware pin state registration sampling
    bool pin3_low = !gpio_get(KEY3_PIN); // Pin 11 (KEY3 - Selection Enter)
    bool pin2_low = !gpio_get(KEY2_PIN); // Pin 12 (KEY2 - Focus Hop Next)
    bool pin1_low = !gpio_get(KEY1_PIN); // Pin 13 (KEY1 - Escape Back)
    bool any_key_pressed = (pin3_low || pin2_low || pin1_low);

    key3_pressed = pin3_low;
    key2_pressed = pin2_low;
    key1_pressed = pin1_low;
    
    if (any_key_pressed && !key_is_locked) {
        printf("[DB_PAD] 2");
    }

    // 2. STATE MACHINE LOGIC: MECHANICAL SWITCH PUSH DOWN
    if (any_key_pressed) {
        if (!key_is_locked) {
            gpio_put(LED_DEBUG_PIN, 0); // Pulled-up: 0 turns debug LED ON
			 
            gpio_set_irq_enabled(KEY1_PIN, GPIO_IRQ_EDGE_FALL, false);
            gpio_set_irq_enabled(KEY2_PIN, GPIO_IRQ_EDGE_FALL, false);
            gpio_set_irq_enabled(KEY3_PIN, GPIO_IRQ_EDGE_FALL, false);
            
            // 🌟 THE PRESS-DOWN GUARD: We pass a raw PRESSED state but set key = 0!
            // This tells LVGL a button was touched, but blocks any premature menu clicks!
            //data->state = LV_INDEV_STATE_PRESSED;
            //data->key = 0;
            key_is_locked = true; 

            // Stash the identity of which key was hit inside our persistent latch
            if (pin3_low)       last_key = LV_KEY_ENTER; 
            else if (pin2_low)  last_key = LV_KEY_NEXT;  
            else if (pin1_low)  last_key = LV_KEY_ESC;  
            
            printf(" [KEY DOWN] .\n\r");
        } else {
            //data->state = LV_INDEV_STATE_RELEASED;
            //data->key = 0; 
        }
    } 
    // 3. 🚀 STATE MACHINE LOGIC: MECHANICAL SWITCH PHYSICAL RELEASE 🚀
    else {
        if (key_is_locked) {
            gpio_put(LED_DEBUG_PIN, 1); // Pulled-up: 1 turns debug LED OFF
            printf(" [KEY UP] .\n\r");

            // Also set your global tracker token matching your existing code paths
            if (last_key == LV_KEY_ENTER)       g_last_physical_key = 10;
            else if (last_key == LV_KEY_NEXT)   g_last_physical_key = 11;
            else if (last_key == LV_KEY_ESC)    g_last_physical_key = 27;
            
            data->state = LV_INDEV_STATE_PRESSED;
            //data->key = last_key; 
            data->key = 0; 


            // Clear hardware interrupt latency states accumulated on pins while held
            gpio_acknowledge_irq(KEY1_PIN, GPIO_IRQ_EDGE_FALL);
            gpio_acknowledge_irq(KEY2_PIN, GPIO_IRQ_EDGE_FALL);
            gpio_acknowledge_irq(KEY3_PIN, GPIO_IRQ_EDGE_FALL);
            
            gpio_set_irq_enabled(KEY1_PIN, GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(KEY2_PIN, GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(KEY3_PIN, GPIO_IRQ_EDGE_FALL, true);
            
            key_is_locked = false; // Reset latch safely for next press pass
            last_key = 0; // Wipe memory clear
            printf("[DB_PAD] 3");
        } else {
            // No button is touched and latch is reset. Tell LVGL everything is clear.
            data->state = LV_INDEV_STATE_RELEASED;
            data->key = 0;
        }
    }
}
