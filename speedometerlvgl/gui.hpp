
#ifndef GUI_HPP
#define GUI_HPP

#include <stdint.h>
#include "lvgl.h"

// Share the navigation group pointer across compilation units
extern lv_group_t * main_menu_nav_group;
extern uint32_t g_last_physical_key;

// Hall-effect wheel speed sensor pulse timing, written by main.cpp's shared IRQ
// callback, read by gui.cpp's speedometer update timer.
extern volatile uint32_t g_hall_last_pulse_us;
extern volatile uint32_t g_hall_last_interval_us;

// Forward declarations for your UI generation modules

void create_menu_icon_widget(lv_obj_t * parent_screen, const lv_image_dsc_t * img_src, int32_t x_pos, int32_t y_pos, int32_t y_trim, int id);
void create_lvgl_home_screen();
void ShowDigitalClock();
void ShowSpeedometerDetails();

#endif // GUI_HPP
