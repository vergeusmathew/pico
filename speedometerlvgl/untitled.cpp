
lv_group_t * settings_submenu_nav_group = nullptr;

void ShowSettingsSubMenu(){
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

    create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_clock_set, 40, 12, 0,0);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_calendar, 180, 12, 0,1);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_bicycle, 40, 130, -1,2);
	create_menu_icon_widget(settings_submenu_screen, &lv_asset_sub_about, 180, 130, +1,3);
	
	lv_obj_t * first_circle = lv_obj_get_child(settings_submenu_screen, 0);
    if (first_circle != nullptr) {
        lv_group_focus_obj(first_circle);
    }
    
    // 5. Connect this screen container to your group so it intercepts KEY1 (ESCAPE)
    lv_group_add_obj(main_menu_nav_group, settings_submenu_screen);
    
    lv_obj_add_event_cb(settings_submenu_screen, [](lv_event_t * e) {
        lv_key_t key = (lv_key_t)lv_event_get_key(e);
        if (key == LV_KEY_ESC) { // Triggered by pressing KEY1
            printf("[GUI NAV] KEY1 (ESC) Clicked inside Sub-Menu! Returning to Home Screen...\n\r");
            create_lvgl_home_screen(); // Unloads sub-menu and restores your 4 main icons
        }
    }, LV_EVENT_KEY, NULL);
    // Force a fresh render cycle pass instantly
    lv_refr_now(NULL);
}
