#
# Copyright 2015 TU Chemnitz
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 2.8.11)
project(uocte)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  configure_file(doc.doxyfile.in doc.doxyfile @ONLY)
  configure_file(intdoc.doxyfile.in intdoc.doxyfile @ONLY)

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/doc.doxyfile > /dev/null
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generating doxygen for core API")

  add_custom_target(intdoc
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/intdoc.doxyfile > /dev/null
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generating doxygen for internals")
endif()

# core files
list(APPEND SOURCES src/core/image.cpp src/core/volume.cpp src/core/oct_data.cpp)

#timeline
list(APPEND SOURCES src/qcustomplot.cpp src/qcustomplot.h)
list(APPEND SOURCES src/timelineWidget.cpp src/timelineWidget.hpp)
list(APPEND SOURCES src/glSectorWidget.cpp src/glSectorWidget.hpp)
list(APPEND SOURCES src/patientFilterWidget.cpp src/patientFilterWidget.hpp)

#converter
list(APPEND SOURCES src/io/tinyxml2.cpp src/io/tinyxml2.h)
list(APPEND SOURCES src/xmlPatientList.cpp src/xmlPatientList.hpp)
list(APPEND SOURCES src/converter.cpp)

#export jpeg
list(APPEND SOURCES src/io/jpge.cpp src/io/jpge.h)
list(APPEND SOURCES src/io/exportJpeg.cpp src/io/exportJpeg.hpp)
list(APPEND SOURCES src/chooseContoursWidget.cpp src/chooseContoursWidget.hpp)

# uoctml loader
find_package(EXPAT REQUIRED QUIET)
include_directories(${EXPAT_INCLUDE_DIRS})
list(APPEND SOURCES src/io/charconv.cpp src/io/file.cpp src/io/load_uoctml.cpp src/io/save_uoctml.cpp src/io/xml.cpp)
list(APPEND LIBRARIES ${EXPAT_LIBRARIES})

# Eyetec loader
find_package(LibArchive QUIET)
if(LibArchive_FOUND)
  include_directories(${LibArchive_INCLUDE_DIRS})
  list(APPEND SOURCES src/io/load_eyetec.cpp)
  list(APPEND LIBRARIES ${LibArchive_LIBRARIES})
else()
  message(WARNING "LibArchive not found. Not building Eyetec Loader.")
endif()

# Heidelberg loader
list(APPEND SOURCES src/io/load_heidelberg.cpp)

# Nidek loader
list(APPEND SOURCES src/io/load_nidek.cpp)

# Topcon loader
find_package(OpenJPEG2 QUIET)
if(OpenJPEG2_FOUND)
  include_directories(${OpenJPEG2_INCLUDE_DIR})
  list(APPEND SOURCES src/io/load_topcon.cpp src/io/j2k_opj2.cpp)
  list(APPEND LIBRARIES ${OpenJPEG2_LIBRARIES})
else()
  find_package(OpenJPEG QUIET)
  if(OpenJPEG_FOUND)
    include_directories(${OpenJPEG_INCLUDE_DIR})
    list(APPEND SOURCES src/io/load_topcon.cpp src/io/j2k_opj.cpp)
    list(APPEND LIBRARIES ${OpenJPEG_LIBRARIES})
  else()
    find_package(Jasper QUIET)
    if (JASPER_FOUND)
      include_directories(${JASPER_INCLUDE_DIR})
      list(APPEND SOURCES src/io/load_topcon.cpp src/io/j2k_jasper.cpp)
      list(APPEND LIBRARIES ${JASPER_LIBRARIES})
    else()
      message(WARNING "No JPEG2000 library found (openjpeg 1.5 or 2.1, jasper). Not building Topcon loader.")
    endif()
  endif()
endif()

# GUI
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
list(APPEND SOURCES src/gl_content.cpp src/main.cpp src/render_fundus.cpp src/render_mip.cpp src/render_sectors.cpp src/render_slice.cpp)
list(APPEND LIBRARIES Qt5::OpenGL Qt5::Widgets Qt5::PrintSupport ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})

set(CMAKE_AUTOMOC ON)
add_executable(uocte ${SOURCES})
target_link_libraries(uocte ${LIBRARIES} )

configure_file(src/mip.vert share/uocte/shader/mip.vert COPYONLY)
configure_file(src/mip.frag share/uocte/shader/mip.frag COPYONLY)

install(TARGETS uocte RUNTIME DESTINATION bin)
install(DIRECTORY ${PROJECT_BINARY_DIR}/share/uocte DESTINATION share)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "an OCT data explorer")
set(CPACK_PACKAGE_VENDOR "TU Chemnitz")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
if(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  #set(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\uocte.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Unified OCT Explorer")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.bitbucket.org\\\\cheine\\\\uocte")
  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.ophthalvis.de")
  set(CPACK_NSIS_CONTACT "Paul Rosenthal <paul.rosenthal@informatik.tu-chemnitz.de>")
  set(CPACK_NSIS_MODIFY_PATH ON)
else(WIN32 AND NOT UNIX)
  set(CPACK_STRIP_FILES "bin/uocte")
  set(CPACK_SOURCE_STRIP_FILES "")
endif(WIN32 AND NOT UNIX)
set(CPACK_PACKAGE_CONTACT "Paul Rosenthal <paul.rosenthal@informatik.tu-chemnitz.de>")
set(CPACK_PACKAGE_EXECUTABLES "uocte" "uocte")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)
