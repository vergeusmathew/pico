#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *outer_border_circle;
    lv_obj_t *inner_border_circle;
    lv_obj_t *arc_behind_scale;
    lv_obj_t *scale_outer0to240;
    lv_obj_t *scale_inner0to150;
    lv_obj_t *arc_speedometer;
    lv_obj_t *label_speed_kmh_1;
    lv_obj_t *label_speed_kmh;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/