# The following variables will be set:
#
# OPENJPEG_FOUND        - Set to true if OpenJPEG can be found
# OPENJPEG_INCLUDE_DIRS - The path to the OpenJPEG header files
# OPENJPEG_LIBRARIES    - The full path to the OpenJPEG library

find_package(PkgConfig QUIET)
pkg_check_modules(PC_OpenJPEG QUIET libopenjpeg1)

find_path(OpenJPEG_INCLUDE_DIR NAMES openjpeg.h PATHS ${PC_OpenJPEG_INCLUDE_DIRS} PATH_SUFFIXES openjpeg-1.5)
find_library(OpenJPEG_LIBRARY NAMES openjpeg PATHS ${PC_OpenJPEG_LIBRARY_DIRS})
set(OpenJPEG_VERSION ${PC_OpenJPEG_VERSION})

find_package_handle_standard_args(OpenJPEG
  FOUND_VAR OpenJPEG_FOUND
  REQUIRED_VARS
    OpenJPEG_LIBRARY
    OpenJPEG_INCLUDE_DIR
  VERSION_VAR OpenJPEG_VERSION
)

if(OpenJPEG_FOUND)
  set(OpenJPEG_LIBRARIES ${OpenJPEG_LIBRARY})
  set(OpenJPEG_INCLUDE_DIRS ${OpenJPEG_INCLUDE_DIR})
  set(OpenJPEG_DEFINITIONS ${PC_OpenJPEG_CFLAGS_OTHER})
endif()

mark_as_advanced(
  OpenJPEG_INCLUDE_DIR
  OpenJPEG_LIBRARY
)
