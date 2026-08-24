#include "gui.hpp"
#include "lv_image_assets.hpp"
#include <stdio.h>
#include "ds3231.hpp"
#include "hardware/sync.h" // REQUIRED FOR SILICON SPINLOCKS
#define WIFI_SPINLOCK_ID 31

static lv_obj_t * lbl_status_text = nullptr;
static lv_timer_t * status_update_timer = nullptr;

// Forward declaration of our new polling worker function
static void status_bar_poller_cb(lv_timer_t * t);

extern Ds3231 rtc; 

extern volatile bool g_wifi_hotspot_available;
extern volatile bool g_rtc_sync_successful;
extern volatile bool g_wifi_scanning_complete;

extern volatile int g_parsed_year;
extern volatile int g_parsed_month;
extern volatile int g_parsed_day;
extern volatile int g_parsed_hour;
extern volatile int g_parsed_minute;
extern volatile int g_parsed_second;
extern volatile bool g_parsed_time_ready_to_load;

extern bool g_core1_is_active; // Link to the core controller flag

static void menu_icon_event_handler(lv_event_t * e);
// Global pointer handles to shift focus groups dynamically between screens
lv_group_t * main_menu_nav_group = nullptr;

// --- 🌟 SCREEN LAYER 1: MAIN HOME SCREEN GRAPHICS SUITE ---
// Add this global label pointer tracking object at the top of your main.cpp file
lv_obj_t * lbl_coordinates_tracker = nullptr;

// Stores which icon ID was actively highlighted before entering a sub-menu
static int g_last_active_home_icon_id = 0; 

void create_menu_icon_widget(lv_obj_t * parent_screen, const lv_image_dsc_t * img_src, int32_t x_pos, int32_t y_pos, int32_t y_trim, int id) {
	// Create the outer white background container circle
    // 3. Set up the target outer background white circle cushion diameter (e.g., 100 pixels)
	int32_t circle_diameter = 100;
    lv_obj_t * white_circle = lv_obj_create(parent_screen);
    lv_obj_set_size(white_circle, circle_diameter, circle_diameter);                       
    lv_obj_set_pos(white_circle, x_pos, y_pos);                          // 📍 Exact position from your blueprint diagram
    
    lv_obj_set_style_border_width(white_circle, 0, 0);
    lv_obj_set_style_pad_all(white_circle, 0, 0); 
    
    lv_obj_set_style_bg_color(white_circle, lv_color_white(), 0);  // Filled white canvas circle
    lv_obj_set_style_bg_opa(white_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(white_circle, LV_RADIUS_CIRCLE, 0);    // Clip corners into circle geometry
    lv_obj_set_scrollbar_mode(white_circle, LV_SCROLLBAR_MODE_OFF);

	lv_obj_set_style_clip_corner(white_circle, true, 0);	
	
	 // Create the image element parented directly inside the circle
    lv_obj_t * img_widget = lv_image_create(white_circle);         
    lv_image_set_src(img_widget, img_src); // 🌟 Dynamically loads passed asset pointer
    
    //lv_obj_center(white_circle);

    // Align the centers automatically and apply the specific vertical asset trim factor
    lv_obj_align(img_widget, LV_ALIGN_CENTER, 0, y_trim);              
    
    lv_obj_set_user_data(white_circle, (void*)(uintptr_t)id); // Set ID tag safely
    lv_obj_add_event_cb(white_circle, menu_icon_event_handler, LV_EVENT_ALL, NULL);

    // Connect this specific widget node to your physical keypad group tracker tracks
    lv_group_add_obj(main_menu_nav_group, white_circle);
    
    lv_obj_set_style_outline_color(white_circle, lv_color_make(255, 255, 0), LV_STATE_FOCUS_KEY); // Crisp Yellow Ring
    lv_obj_set_style_outline_width(white_circle, 3, LV_STATE_FOCUS_KEY);                          // 3-pixel wide outline
    lv_obj_set_style_outline_pad(white_circle, 3, LV_STATE_FOCUS_KEY);                            // Offset pad cushion space
    
    //lv_obj_set_style_outline_width(white_circle, 0, LV_STATE_FOCUS_KEY);
}	


static void status_bar_poller_cb(lv_timer_t * t) {
    
    // Local snapshot variables
    bool local_time_ready = false;
    bool local_scan_complete = false;
    bool local_hotspot_ok = false;
    bool local_sync_ok = false;

    // CORE 0 SPINLOCK CHECK: Lock out Core 1 for a few microseconds to grab a safe RAM snapshot!
    if (g_core1_is_active) {
        uint32_t irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
        local_time_ready    = g_parsed_time_ready_to_load;
        local_scan_complete = g_wifi_scanning_complete;
        local_hotspot_ok    = g_wifi_hotspot_available;
        local_sync_ok       = g_rtc_sync_successful;
        if (g_parsed_time_ready_to_load) g_parsed_time_ready_to_load = false; 
        spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), irq_status);
    } else {
        local_time_ready    = g_parsed_time_ready_to_load;
        local_scan_complete = g_wifi_scanning_complete;
        local_hotspot_ok    = g_wifi_hotspot_available;
        local_sync_ok       = g_rtc_sync_successful;
        if (g_parsed_time_ready_to_load) g_parsed_time_ready_to_load = false; 
    }

    // Now process the extracted snapshot parameters on Core 0 with zero cross-core collisions!
    if (local_time_ready) {
        printf("[DB_GUI] 1");
        rtc.SetTime((uint8_t)g_parsed_hour, (uint8_t)g_parsed_minute, (uint8_t)g_parsed_second);
        rtc.SetDate((uint8_t)g_parsed_day,  (uint8_t)g_parsed_month,  (uint16_t)g_parsed_year);
        printf("[DB_GUI] 2");
    }

    if (local_scan_complete && lbl_status_text != nullptr) {
       printf("[DB_GUI] 3");
       lv_obj_t * status_container = lv_obj_get_parent(lbl_status_text);
        if (status_container != nullptr) {
            
            lv_obj_delete(lbl_status_text);
            lbl_status_text = nullptr;

            if (!local_hotspot_ok) {
                lv_obj_t * lbl_no_wifi = lv_label_create(status_container);
                lv_obj_set_style_text_color(lbl_no_wifi, lv_color_make(255, 0, 0), 0); 
                lv_obj_set_pos(lbl_no_wifi, 55, 2); 
                lv_label_set_text(lbl_no_wifi, "[!W]"); 

                lv_obj_t * lbl_no_time = lv_label_create(status_container);
                lv_obj_set_style_text_color(lbl_no_time, lv_color_make(255, 100, 0), 0); 
                lv_obj_align_to(lbl_no_time, lbl_no_wifi, LV_ALIGN_OUT_LEFT_MID, -8, 0); 
                lv_label_set_text(lbl_no_time, "[!T]"); 
            } 
            else if (local_hotspot_ok && !local_sync_ok) {
                lv_obj_t * lbl_wifi_ok = lv_label_create(status_container);
                lv_obj_set_style_text_color(lbl_wifi_ok, lv_color_make(0, 255, 0), 0); 
                lv_obj_set_pos(lbl_wifi_ok, 55, 2);
                lv_label_set_text(lbl_wifi_ok, "[W]");

                lv_obj_t * lbl_no_time = lv_label_create(status_container);
                lv_obj_set_style_text_color(lbl_no_time, lv_color_make(255, 0, 0), 0); 
                lv_obj_align_to(lbl_no_time, lbl_wifi_ok, LV_ALIGN_OUT_LEFT_MID, -8, 0);
                lv_label_set_text(lbl_no_time, "[!T]");
            }
            else {
                lv_obj_t * lbl_healthy = lv_label_create(status_container);
                lv_obj_set_style_text_color(lbl_healthy, lv_color_make(0, 255, 255), 0); 
                lv_obj_set_pos(lbl_healthy, 30, 2);
                lv_label_set_text(lbl_healthy, "[okay]"); 
            }
        }

        lv_timer_delete(status_update_timer);
        status_update_timer = nullptr;
        printf("[DB_GUI] 4");
    }
}


static lv_obj_t * lbl_small_time = nullptr;
static lv_obj_t * lbl_small_date = nullptr;
static lv_obj_t * lbl_clock_hours = nullptr;
static lv_obj_t * lbl_clock_colon = nullptr;
static lv_obj_t * lbl_clock_minutes = nullptr;
static lv_timer_t * clock_blink_timer = nullptr;
static bool hrlen = false;
 
void create_lvgl_home_screen() {
	// Save a handle to the current active screen right before we overwrite it
	if (clock_blink_timer != nullptr) {
        lv_timer_delete(clock_blink_timer);
        clock_blink_timer = nullptr;
        printf("[GUI SYS] clock_blink_timer del.\n\r");
    }
    
    lv_obj_t * old_screen = lv_screen_active();
    
    // 1. Reset focus group to clear layout hooks safely
    if (main_menu_nav_group == nullptr) {
        main_menu_nav_group = lv_group_create();
        lv_indev_set_group(lv_indev_get_next(NULL), main_menu_nav_group);
    } else {
        lv_group_remove_all_objs(main_menu_nav_group);
    }

    // 2. Clear canvas module matching full screen resolution limits
    lv_obj_t * home_screen = lv_obj_create(NULL);
    lv_obj_set_size(home_screen, 320, 240);
    lv_obj_set_scrollbar_mode(home_screen, LV_SCROLLBAR_MODE_OFF);
    
    lv_obj_set_style_pad_all(home_screen, 0, 0);       
    lv_obj_set_style_border_width(home_screen, 0, 0);   
    lv_obj_set_style_radius(home_screen, 0, 0);         
    
    // Arguments are: (object, color/opacity, selector flag state)
    lv_obj_set_style_bg_color(home_screen, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_bg_opa(home_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); 
    
      
    lv_screen_load(home_screen); // Point layout engine straight to this screen
    
    // 🚀 CRUCIAL MEMORY FIX: Cleanly delete the old sub-settings screen layer!
    if (old_screen != nullptr) {
        lv_obj_delete_async(old_screen);
    }

    create_menu_icon_widget(home_screen, &lv_asset_speedometer, 40, 12, -3,0);
	create_menu_icon_widget(home_screen, &lv_asset_main_clock, 180, 12, 0,1);
	create_menu_icon_widget(home_screen, &lv_asset_graph, 40, 130, -1,2);
	create_menu_icon_widget(home_screen, &lv_asset_main_settings, 180, 130, +1,3);
	
	// Create status notifications container block at extreme top right corner
    lv_obj_t * status_container = lv_obj_create(home_screen);
    lv_obj_set_size(status_container, 80, 25);
    lv_obj_set_pos(status_container, 235, 5); // Extreme Top-Right Corner position bounding boxes
    lv_obj_set_scrollbar_mode(status_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(status_container, LV_OPA_TRANSP, 0); // Transparent canvas frame
    lv_obj_set_style_border_width(status_container, 0, 0);
    lv_obj_set_style_pad_all(status_container, 0, 0);
    
    //  LOCAL SNAPSHOT REGISTERS FOR THE HOME SCREEN INITIAL GENERATOR
    bool local_scan_complete = false;
    bool local_hotspot_ok = false;
    bool local_sync_ok = false;
	// THE DEADLOCK ANTIDOTE: Only engage the spinlock if Core 1 is actually alive!
	if (g_core1_is_active) {
		//  SECURE SPINLOCK SNAPSHOT PASS: Lock out Core 1 for a few microseconds
		uint32_t home_irq_status = spin_lock_blocking(spin_lock_instance(WIFI_SPINLOCK_ID));
		local_scan_complete = g_wifi_scanning_complete;
		local_hotspot_ok    = g_wifi_hotspot_available;
		local_sync_ok       = g_rtc_sync_successful;
		spin_unlock(spin_lock_instance(WIFI_SPINLOCK_ID), home_irq_status); // Release lock immediately!
	} else {
        // If Core 1 is offline, read variables directly with 100% thread safety!
        local_scan_complete = g_wifi_scanning_complete;
        local_hotspot_ok    = g_wifi_hotspot_available;
        local_sync_ok       = g_rtc_sync_successful;
    }

    // Evaluate the condition using the safe thread-isolated variables
    if (!local_scan_complete) {
		printf("[GUI SYS] 1");
        // Core 1 is still working in the background. Draw temporary yellow [SCAN]
        lbl_status_text = lv_label_create(status_container);
        lv_obj_set_pos(lbl_status_text, 10, 2);
        lv_obj_set_style_text_color(lbl_status_text, lv_color_make(200, 200, 0), 0); 
        lv_label_set_text(lbl_status_text, "[SCAN]"); 

        // SPAWN THE TIMER TO MONITOR ASYNC PROGRESS EVERY 200MS
        if (status_update_timer == nullptr) {
			printf("[GUI SYS] 2");
            status_update_timer = lv_timer_create(status_bar_poller_cb, 200, NULL);
        }
    } else {
        // Core 1 already finished previously! Draw the definitive healthy statuses directly
        if (!local_hotspot_ok) {
            lv_obj_t * lbl_no_wifi = lv_label_create(status_container);
            lv_obj_set_style_text_color(lbl_no_wifi, lv_color_make(255, 0, 0), 0); 
            lv_obj_set_pos(lbl_no_wifi, 55, 2); 
            lv_label_set_text(lbl_no_wifi, "[!W]"); 

            lv_obj_t * lbl_no_time = lv_label_create(status_container);
            lv_obj_set_style_text_color(lbl_no_time, lv_color_make(255, 100, 0), 0); 
            lv_obj_align_to(lbl_no_time, lbl_no_wifi, LV_ALIGN_OUT_LEFT_MID, -8, 0); 
            lv_label_set_text(lbl_no_time, "[!T]"); 
        } 
        else if (local_hotspot_ok && !local_sync_ok) {
            lv_obj_t * lbl_wifi_ok = lv_label_create(status_container);
            lv_obj_set_style_text_color(lbl_wifi_ok, lv_color_make(0, 255, 0), 0); 
            lv_obj_set_pos(lbl_wifi_ok, 55, 2);
            lv_label_set_text(lbl_wifi_ok, "[W]");

            lv_obj_t * lbl_no_time = lv_label_create(status_container);
            lv_obj_set_style_text_color(lbl_no_time, lv_color_make(255, 0, 0), 0); 
            lv_obj_align_to(lbl_no_time, lbl_wifi_ok, LV_ALIGN_OUT_LEFT_MID, -8, 0);
            lv_label_set_text(lbl_no_time, "[!T]");
        }
        else {
            lv_obj_t * lbl_healthy = lv_label_create(status_container);
            lv_obj_set_style_text_color(lbl_healthy, lv_color_make(0, 255, 255), 0); 
            lv_obj_set_pos(lbl_healthy, 30, 2);
            lv_label_set_text(lbl_healthy, "[OKay]"); 
        }
    }
	
	// THE SYNC ENGINE: DYNAMICALLY RESTORE ICON LOOP STRUCTURE 
    lv_obj_t * object_to_focus = nullptr;
    
    // Scan all children inside your fresh home_screen container layer
    uint32_t child_count = lv_obj_get_child_cnt(home_screen);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t * child = lv_obj_get_child(home_screen, i);
        if (child != nullptr) {
            uintptr_t child_id = (uintptr_t)lv_obj_get_user_data(child);
            
            // Check if this newly generated icon container matches our pre-exit anchor ID
            if ((int)child_id == g_last_active_home_icon_id) {
                object_to_focus = child;
                break;
            }
        }
    }

    // Fallback: If no match was found, anchor safely to the very first child container slot
    if (object_to_focus == nullptr) {
        object_to_focus = lv_obj_get_child(home_screen, 0);
    }

    // Pass the specific target handle straight into your navigation tracking group loops
    if (object_to_focus != nullptr) {
        lv_group_focus_obj(object_to_focus);
        printf(" Restored ring to Icon ID: %d\n\r", 
               (int)(uintptr_t)lv_obj_get_user_data(object_to_focus));
    }    
    /*
       lv_obj_t * first_circle = lv_obj_get_child(home_screen, 0);
       if (first_circle != nullptr) {
        lv_group_focus_obj(first_circle);
    }*/
    
    if (main_menu_nav_group != nullptr) {
        lv_group_set_editing(main_menu_nav_group, false);
    }
    // Force a fresh render cycle pass instantly
    lv_refr_now(NULL);
    
    //THE POST-CLOCK INPUT DEVICE CHANNEL RECOVERY FIX 
    // ========================================================================
    // Force LVGL to clear any stale key tracking states left behind by the deleted clock screen.
    // This tells the engine to immediately resume polling your my_keypad_read_cb loop pass!
    lv_indev_t * indev = lv_indev_get_next(NULL); // Grabs your active Keypad input channel handle
    if (indev != nullptr) {
        lv_indev_reset(indev, NULL); // Resets the input channel state safely
    }
}


lv_group_t * settings_submenu_nav_group = nullptr;

void ShowSettingsSubMenu(){
	// Save a handle to the current active screen right before we overwrite it
    lv_obj_t * old_screen = lv_screen_active();
    
    // 1. Wipe out all previous object references from the shared navigation group
	lv_group_remove_all_objs(main_menu_nav_group);
	
	// 2. Clear canvas module matching full screen resolution limits
    lv_obj_t * settings_submenu_screen = lv_obj_create(NULL);
    lv_obj_set_size(settings_submenu_screen, 320, 240);
    lv_obj_set_scrollbar_mode(settings_submenu_screen, LV_SCROLLBAR_MODE_OFF);
    
    lv_obj_set_style_pad_all(settings_submenu_screen, 0, 0);       
    lv_obj_set_style_border_width(settings_submenu_screen, 0, 0);   
    lv_obj_set_style_radius(settings_submenu_screen, 0, 0);         
    
        // 🌟 DIRECT STYLE METHOD: Set properties straight onto the home_screen object!
    // Arguments are: (object, color/opacity, selector flag state)
    lv_obj_set_style_bg_color(settings_submenu_screen, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_bg_opa(settings_submenu_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT); 

	lv_screen_load(settings_submenu_screen); // Point layout engine straight to this screen
	
	// This stops ghost layers from trapping your physical keypad focus inputs.
    if (old_screen != nullptr) {
        lv_obj_delete(old_screen);
    }

    create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_clock_set, 40, 12, 0,10);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_calendar, 180, 12, 0,11);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_bicycle, 40, 130, -1,12);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_about, 180, 130, +1,13);
	
	lv_obj_t * first_circle = lv_obj_get_child(settings_submenu_screen, 1);
    if (first_circle != nullptr) {
        lv_group_focus_obj(first_circle);
    }
    
    // Force a fresh render cycle pass instantly
    lv_refr_now(NULL);
}


// 🌟 THE COMPACT BLINKING TIME CONTROLLER CALLBACK 🌟
static void clock_timer_cb(lv_timer_t * timer) {
	//If the home screen just loaded and wiped out our labels, 
	if (lbl_clock_hours == nullptr || lbl_clock_colon == nullptr || lbl_clock_minutes == nullptr) {
        return; 
    }
    static bool show_colon = true;
    //char temp_buffer[10];
    
    char hour_buf[16];
    char min_buf[16];
    char time_buf[32];
    char date_buf[32];
	
    RtcTime current_time = rtc.GetTime(); // Queries the DS3231 I2C tracking register map
    int hours = current_time.hour;        // Extract hour value
    int minutes = current_time.min;       // Extract minute value
	int seconds = current_time.sec;       // Extract seconds
    int day     = current_time.day;       // Extract calendar day
    int month   = current_time.month;     // Extract calendar month
    int year    = current_time.year % 100; 
	//hours = 13;
	//minutes = 37;
 // 2. Compute your working 12-hour format structure logic
    if (hours == 0) hours = 12;
    else if (hours > 12) {
		hours -= 12;
	}
	
	// 1. Update Hours
    if (lbl_clock_hours != nullptr) {
        sprintf(hour_buf, "%d", hours);
        lv_label_set_text(lbl_clock_hours, hour_buf);
    }

    // 2. Update Minutes
    if (lbl_clock_minutes != nullptr) {
        sprintf(min_buf, "%02d", minutes);
        lv_label_set_text(lbl_clock_minutes, min_buf);
    }

	if (hours >= 0 && hours < 10) {
        //printf("[CLOCK TICK] Single Digit Hour Match! Framing: %d : %02d\n\r", hours,minutes);
        
        // 🌟 SINGLE DIGIT OPTIMIZATION: Move the "3" right up near the colon!
        if (lbl_clock_hours != nullptr && lbl_clock_colon != nullptr) {
            lv_obj_align_to(lbl_clock_hours, lbl_clock_colon, LV_ALIGN_OUT_LEFT_MID, -47, 6);   //35 29
        }
    } else {
        //printf("[CLOCK TICK] Double Digit Hour Match! Framing: %d : %02d\n\r", hours,minutes);
        
        // 🌟 DOUBLE DIGIT OPTIMIZATION: Expand leftward to prevent overlap cramming!
        if (lbl_clock_hours != nullptr && lbl_clock_colon != nullptr) {
            lv_obj_align_to(lbl_clock_hours, lbl_clock_colon, LV_ALIGN_OUT_LEFT_MID, -57, 6);
        }
    }

    // 3. Toggle only the central colon's visibility to prevent number clipping!
    if (lbl_clock_colon != nullptr) {
         lv_obj_set_hidden(lbl_clock_colon, !show_colon);
    }

    show_colon = !show_colon; // Toggle colon state
    // A. Formats the absolute tracking time string -> "hh:mm:ss" (Uses raw 24h format or 12h)
    if (lbl_small_time != nullptr) {
        sprintf(time_buf, "%02d:%02d:%02d", hours, minutes, seconds);
        lv_label_set_text(lbl_small_time, time_buf);
    }

    // B. Formats the absolute calendar date string -> "dd/mm/yy"
    if (lbl_small_date != nullptr) {
        sprintf(date_buf, "%02d/%02d/%02d", day, month, year);
        lv_label_set_text(lbl_small_date, date_buf);
    }
}
extern bool g_request_indev_reset;
void ShowDigitalClock() {
	 if (clock_blink_timer != nullptr) {
        printf("In ShowDigitalClock!\n\r");
        return;
    }

    //printf("\n\r>>> [SCREEN ENTRY] \n\r");
    
	lv_obj_t * old_screen = lv_screen_active();
    // 1. Wipe out navigation hooks from the old main menu layout cleanly
    lv_group_remove_all_objs(main_menu_nav_group);

    // 2. Instantiate a fresh, independent screen module canvas base
    lv_obj_t * clock_screen = lv_obj_create(NULL);
    lv_obj_set_size(clock_screen, 320, 240);
    lv_obj_set_scrollbar_mode(clock_screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(clock_screen, 0, 0);
    lv_obj_set_style_border_width(clock_screen, 0, 0);
    
    // Deep black canvas background
    lv_obj_set_style_bg_color(clock_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(clock_screen, LV_OPA_COVER, 0);
    lv_screen_load(clock_screen);

	if (old_screen != nullptr) {
        lv_obj_delete(old_screen);
    }
    
    // 3. TITLE DESCRIPTOR LABEL
    lv_obj_t * lbl_clock_title = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(lbl_clock_title, lv_color_make(120, 120, 140), 0); // Gray text
    lv_obj_set_pos(lbl_clock_title, 20, 15);
    //lv_label_set_text(lbl_clock_title, "RTC DIGITAL TIMEREADOUT");
    lv_label_set_text(lbl_clock_title, " ");

    // ========================================================================
    // 🌟 SPREAD ALIGNMENT: THREE INTERLINKED LABELS USING 48PX VECTOR FONT 🌟
    // ========================================================================
    
    // A. CENTRAL COLON WIDGET (Anchored exactly in the middle of the display)
    lbl_clock_colon = lv_label_create(clock_screen);
    //lv_obj_set_style_text_color(lbl_clock_colon, lv_color_make(0, 255, 255), 0); // Cyan theme
    lv_obj_set_style_text_color(lbl_clock_colon, lv_color_make(0, 140, 50), 0); // Dim Emerald Green theme
    lv_obj_set_style_text_font(lbl_clock_colon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_scale(lbl_clock_colon, 512, 0); // Scale up to 96px tall!
    lv_label_set_text(lbl_clock_colon, ":");
    lv_obj_align(lbl_clock_colon, LV_ALIGN_CENTER, -14, -31); // 25 Dead center offset anchor
    
    // B. HOURS DISPLAY ELEMENT (Positioned to the left of the central colon)
    lbl_clock_hours = lv_label_create(clock_screen);
    //lv_obj_set_style_text_color(lbl_clock_hours, lv_color_make(0, 255, 255), 0);
    lv_obj_set_style_text_color(lbl_clock_hours, lv_color_make(0, 140, 50), 0); // Dim Emerald Green theme
    lv_obj_set_style_text_font(lbl_clock_hours, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_scale(lbl_clock_hours, 512, 0); // Scale up to 96px tall!
    lv_label_set_text(lbl_clock_hours, " ");
	//lv_obj_align_to(lbl_clock_hours, lbl_clock_colon, LV_ALIGN_OUT_LEFT_MID, -57, 6);  //53


    // C. MINUTES DISPLAY ELEMENT (Positioned to the right of the central colon)
    lbl_clock_minutes = lv_label_create(clock_screen);
    //lv_obj_set_style_text_color(lbl_clock_minutes, lv_color_make(0, 255, 255), 0);
    lv_obj_set_style_text_color(lbl_clock_minutes, lv_color_make(0, 140, 50), 0); // Dim Emerald Green theme
    lv_obj_set_style_text_font(lbl_clock_minutes, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_scale(lbl_clock_minutes, 512, 0); // Scale up to 96px tall!
    lv_label_set_text(lbl_clock_minutes, " ");
    lv_obj_align_to(lbl_clock_minutes, lbl_clock_colon, LV_ALIGN_OUT_RIGHT_MID, 27, 6);  //31
    
    // 1. SMALL TIME LABEL (hh:mm:ss)
    lbl_small_time = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(lbl_small_time, lv_color_make(120, 120, 140), 0); // High contrast solid white
    lv_obj_set_style_text_font(lbl_small_time, &lv_font_montserrat_14, 0); // 14px crisp font
    
    // Explicitly lock coordinates matching your original Y=213 blueprint placement
    lv_obj_set_pos(lbl_small_time, 245, 205); 
    lv_label_set_text(lbl_small_time, "   ");

    // 2. SMALL DATE LABEL (dd/mm/yy)
    lbl_small_date = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(lbl_small_date,lv_color_make(120, 120, 140) , 0); // High contrast solid white lv_color_white()
    lv_obj_set_style_text_font(lbl_small_date, &lv_font_montserrat_14, 0); // 14px crisp font
    
    // Explicitly lock coordinates matching your original Y=225 blueprint placement
    lv_obj_set_pos(lbl_small_date, 245, 220); 
    lv_label_set_text(lbl_small_date, "   ");
    // ========================================================================

    // 4. Initialize the background task blink timer wheel (Fires every 500ms)
    clock_blink_timer = lv_timer_create(clock_timer_cb, 500, NULL);

    // 5. Hook up the escape button listener to return to home menu safely
    lv_group_add_obj(main_menu_nav_group, clock_screen);
    lv_group_focus_obj(clock_screen);
    
    lv_obj_add_event_cb(clock_screen, [](lv_event_t * e) {
        lv_event_code_t code = lv_event_get_code(e);
        
        if (code >= LV_EVENT_PRESSED && code <= LV_EVENT_KEY) {
			printf("[CLK SCRN] Code: %d | Key: %d\n\r", 
           (int)code, (int)g_last_physical_key);
		}
        
        if (code == LV_EVENT_KEY) {
            
            // Verify that the button released was actually KEY1 (Token 27)
            if (g_last_physical_key == 27) {
				g_last_physical_key = 0;
            // Delete background timer memory to avoid unexpected crash conditions
				if (clock_blink_timer != nullptr) {
					printf("Esc Home \n\r");

                    g_request_indev_reset = true;
					lv_timer_pause(clock_blink_timer);
					lv_group_remove_obj((lv_obj_t *)lv_event_get_current_target(e));
					clock_blink_timer = nullptr;
					lbl_clock_hours = nullptr;
					lbl_clock_colon = nullptr;
					lbl_clock_minutes = nullptr;
					lbl_small_time = nullptr;
					lbl_small_date = nullptr;
					 // Clear the global tracker variable clean for the next press pass
                    
					create_lvgl_home_screen();
				}
            }
		}
    }, LV_EVENT_ALL, NULL);
}



static void menu_icon_event_handler(lv_event_t * e) {
    lv_obj_t * target_circle = (lv_obj_t *)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    uintptr_t icon_id = (uintptr_t)lv_obj_get_user_data(target_circle);
    
    if (code >= LV_EVENT_PRESSED && code <= LV_EVENT_KEY) {
        printf("[HANDL] ID: %d |Code: %d | Key:%d  \n\r", 
               (int)icon_id, (int)code, (int)g_last_physical_key);
    }
    
    // We wait until the finger completely leaves the switch face!
    if (code == LV_EVENT_KEY) {
        if (g_last_physical_key == 10 ) {  
			g_last_physical_key = 0;
			g_last_active_home_icon_id = (int)icon_id;
			printf(" Saved ID:%d  \n\r", g_last_active_home_icon_id);
            //printf(" >> [RELEASE CLICK ACTION] KEY3 released on Icon ID: %d\n\r", (int)icon_id);
			// Main Home Screen Selections
			if (icon_id == 1) { // Clock ID
				//printf(" -> Route To: ShowDigitalClock()\n\r");
				ShowDigitalClock();
			}
			// else if (icon_id == 0) { ShowSpeedometerDetails(); }
			// else if (icon_id == 2) { ShowGraphDetails(); }
			else if (icon_id == 3) { 
				//printf(" -> Route To: ShowSettingsMenu()\n\r");
				ShowSettingsSubMenu(); 
			}
			// SETTINGS SUB-MENU ROUTING PATHS (IDs 10 to 13)
			else if (icon_id == 10) { // Sub-Option: Clock Set
				//printf("[SUB-SETTINGS] KEY3 (ENTER) Clicked on Clock Set (ID: 10)!\n\r");
				// RunClockSettingWorkspace(); // Example target placeholder
			}
			else if (icon_id == 11) { // Sub-Option: Calendar Set
				printf("[SUB-SETTINGS] KEY3 (ENTER) Clicked on Calendar Set (ID: 11)!\n\r");
			}
			else if (icon_id == 12) { // Sub-Option: Wheel Size Calibration
				printf("[SUB-SETTINGS] KEY3 (ENTER) Clicked on Bicycle Set (ID: 12)!\n\r");
			}
			else if (icon_id == 13) { // Sub-Option: System Information
				printf("[SUB-SETTINGS] KEY3 (ENTER) Clicked on About Panel (ID: 13)!\n\r");
			}
		}	
		else if (g_last_physical_key == 11) {
            // 🚀 THE STUCK RING FIX: Reset the global tracking latch to 0 instantly!
            g_last_physical_key = 0; 
            if (main_menu_nav_group != nullptr) {
                lv_group_focus_next(main_menu_nav_group);
				lv_obj_t * active_focused_obj = lv_group_get_focused(main_menu_nav_group);
                
                if (active_focused_obj != nullptr) {
                    uintptr_t real_active_id = (uintptr_t)lv_obj_get_user_data(active_focused_obj);
                    
                    // 🌟 This will output your terminal traces in perfect lockstep with your eyes!
                    printf(" K2-11 Hop. Ring on Icn ID: %d\n\r", (int)real_active_id);
                }
            }    
        }
        else if (g_last_physical_key == 27 ) {
			g_last_physical_key = 0;
            printf(" K1-27 done, Icon ID: %d\n\r", (int)icon_id);
            
            if (icon_id >= 10 && icon_id <= 13) {
                printf("[GUI NAV] Leaving Sub-Menu safely on release. Loading Home...\n\r");
                create_lvgl_home_screen(); 
            }
        }
    }
}



