#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL2::SDL2" for configuration "Release"
set_property(TARGET SDL2::SDL2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL2::SDL2 PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libSDL2-2.0.so.0.2600.2"
  IMPORTED_SONAME_RELEASE "libSDL2-2.0.so.0"
  )

list(APPEND _cmake_import_check_targets SDL2::SDL2 )
list(APPEND _cmake_import_check_files_for_SDL2::SDL2 "${_IMPORT_PREFIX}/lib/libSDL2-2.0.so.0.2600.2" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
