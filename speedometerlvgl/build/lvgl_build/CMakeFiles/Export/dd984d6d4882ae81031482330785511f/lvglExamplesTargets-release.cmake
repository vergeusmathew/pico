#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "lvgl::lvgl_examples" for configuration "Release"
set_property(TARGET lvgl::lvgl_examples APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lvgl::lvgl_examples PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/liblvgl_examples.a"
  )

list(APPEND _cmake_import_check_targets lvgl::lvgl_examples )
list(APPEND _cmake_import_check_files_for_lvgl::lvgl_examples "${_IMPORT_PREFIX}/lib/liblvgl_examples.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
