#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unity" for configuration ""
set_property(TARGET unity APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(unity PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "/home/felix/Projects/Coursera/linux_systemprogrammierung_einfuehrung_buildroot/assignment-autotest/Unity/src/libunity.a"
  )

list(APPEND _cmake_import_check_targets unity )
list(APPEND _cmake_import_check_files_for_unity "/home/felix/Projects/Coursera/linux_systemprogrammierung_einfuehrung_buildroot/assignment-autotest/Unity/src/libunity.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
