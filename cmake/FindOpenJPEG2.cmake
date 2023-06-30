# The following variables will be set:
#
# OPENJPEG2_FOUND        - Set to true if OpenJPEG v2 can be found
# OPENJPEG2_INCLUDE_DIRS - The path to the OpenJPEG v2 header files
# OPENJPEG2_LIBRARIES    - The full path to the OpenJPEG v2 library

find_package(PkgConfig QUIET)
pkg_check_modules(PC_OpenJPEG2 QUIET libopenjp2)

find_path(OpenJPEG2_INCLUDE_DIR NAMES openjpeg.h PATHS ${PC_OpenJPEG2_INCLUDE_DIRS} PATH_SUFFIXES openjpeg-2.1)
find_library(OpenJPEG2_LIBRARY NAMES openjp2 PATHS ${PC_OpenJPEG2_LIBRARY_DIRS})
set(OpenJPEG2_VERSION ${PC_OpenJPEG2_VERSION})

find_package_handle_standard_args(OpenJPEG2
  FOUND_VAR OpenJPEG2_FOUND
  REQUIRED_VARS
    OpenJPEG2_LIBRARY
    OpenJPEG2_INCLUDE_DIR
  VERSION_VAR OpenJPEG2_VERSION
)

if(OpenJPEG2_FOUND)
  set(OpenJPEG2_LIBRARIES ${OpenJPEG2_LIBRARY})
  set(OpenJPEG2_INCLUDE_DIRS ${OpenJPEG2_INCLUDE_DIR})
  set(OpenJPEG2_DEFINITIONS ${PC_OpenJPEG2_CFLAGS_OTHER})
endif()

mark_as_advanced(
  OpenJPEG2_INCLUDE_DIR
  OpenJPEG2_LIBRARY
)
