
#ifndef LV_IMAGE_ASSETS_HPP
#define LV_IMAGE_ASSETS_HPP

#include "lvgl.h"
#include "SpeedDial_70x62.h"
#include "clock15_70x70.h"
#include "Stats_70x70.h"
#include "Settingsicon70x68.h"

#include "AdjustClock_70x70.h"
#include "Calender70x68.h"
#include "Tyre70x43.h"
#include "About64_70.h"

// 1. Tell the compiler your raw legacy arrays exist somewhere in your project modules
//extern "C" {
    extern const unsigned short SpeedDialimg[];        // 70 x 62
    extern const unsigned short Clockimg[];          // 70 x 70
    extern const unsigned short Statsimg[];            // 70 x 70
    extern const unsigned short Settingsimg[];     // 70 x 68

    /*extern const unsigned short AdjClkimg[];      // 70 x 70
     extern const unsigned short Calenderimg[];      // 70 x 68
    extern const unsigned short Tyreimg[];             // 70 x 43
    extern const unsigned short Aboutimg[];          // 70 x 70
    */
//}

// ----------------------------------------------------------------------------
// 2. WRAP YOUR EXISTING MAIN HOME SCREEN ICON ASSETS FOR LVGL NATIVELY
// ----------------------------------------------------------------------------

inline const lv_image_dsc_t lv_asset_speedometer = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, // Enforces 16-bit RGB565 color format layout natively
        .flags = 0,
        .w = 70, 
        .h = 62, 
        .stride = 70 * 2, // FIXED FOR v9.1: Stride tells the engine width * bytes-per-pixel (70 * 2 = 140)
    },
    .data_size = 70 * 62 * 2, // Total bytes size footprint (Width * Height * 2 bytes)
    .data = (const uint8_t*)SpeedDialimg,
};

inline const lv_image_dsc_t lv_asset_main_clock = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 70, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 70 * 2,
    .data = (const uint8_t*)Clockimg,
};

inline const lv_image_dsc_t lv_asset_graph = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 70, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 70 * 2,
    .data = (const uint8_t*)Statsimg,
};

inline const lv_image_dsc_t lv_asset_main_settings = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 68, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 68 * 2,
    .data = (const uint8_t*)Settingsimg,
};

// --- SUBMENU IMAGES ---

inline const lv_image_dsc_t lv_asset_sub_clock_set = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 70, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 70 * 2,
    .data = (const uint8_t*)AdjClkimg,
};


inline const lv_image_dsc_t lv_asset_sub_calendar = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 68, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 68 * 2,
    .data = (const uint8_t*)Calenderimg,
};

inline const lv_image_dsc_t lv_asset_sub_bicycle = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 70, 
        .h = 43, 
        .stride = 70 * 2,
    },
    .data_size = 70 * 43 * 2,
    .data = (const uint8_t*)Tyreimg,
};


inline const lv_image_dsc_t lv_asset_sub_about = {
    .header = {
        .magic = LV_IMAGE_SRC_VARIABLE,
        .cf = LV_COLOR_FORMAT_RGB565, 
        .flags = 0,
        .w = 64, 
        .h = 70, 
        .stride = 64 * 2,
    },
    .data_size = 64 * 70 * 2,
    .data = (const uint8_t*)Aboutimg,
};

#endif
