#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "kcp::kcp" for configuration ""
set_property(TARGET kcp::kcp APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(kcp::kcp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libkcp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS kcp::kcp )
list(APPEND _IMPORT_CHECK_FILES_FOR_kcp::kcp "${_IMPORT_PREFIX}/lib/libkcp.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
