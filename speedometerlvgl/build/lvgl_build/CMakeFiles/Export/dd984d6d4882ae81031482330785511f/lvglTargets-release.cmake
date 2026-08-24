#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lvgl::lvgl" for configuration "Release"
set_property(TARGET lvgl::lvgl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lvgl::lvgl PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "ASM;C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/liblvgl.a"
  )

list(APPEND _cmake_import_check_targets lvgl::lvgl )
list(APPEND _cmake_import_check_files_for_lvgl::lvgl "${_IMPORT_PREFIX}/lib/liblvgl.a" )

# Import target "lvgl::lvgl_thorvg" for configuration "Release"
set_property(TARGET lvgl::lvgl_thorvg APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lvgl::lvgl_thorvg PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/liblvgl_thorvg.a"
  )

list(APPEND _cmake_import_check_targets lvgl::lvgl_thorvg )
list(APPEND _cmake_import_check_files_for_lvgl::lvgl_thorvg "${_IMPORT_PREFIX}/lib/liblvgl_thorvg.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
