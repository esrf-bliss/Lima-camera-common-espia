###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2019
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9
#  FRANCE
#
#  Contact: lima@esrf.fr
#
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

cmake_minimum_required(VERSION 3.1)

project(espia)

# Include additional modules that are used inconditionnaly
include(GNUInstallDirs)
include(GenerateExportHeader)

# If conda build, always set lib dir to 'lib'
if($ENV{CONDA_BUILD})
  set(CMAKE_INSTALL_LIBDIR "lib")
endif()

# Set lower / upper case project names
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)

# Set targets export name (used by lima and dependencies)
set(TARGETS_EXPORT_NAME "${PROJECT_NAME_LOWER}-targets")
#set(namespace "${PROJECT_NAME}::")

# Check if project is being used directly or via add_subdirectory
set(CAMERA_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CAMERA_MASTER_PROJECT ON)
endif()

if (CAMERA_MASTER_PROJECT)
  find_package(Lima REQUIRED COMPONENTS devel tools)
endif()

# CMake additional macros
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${LIMA_CMAKE_INCLUDE_DIRS} ${CMAKE_MODULE_PATH})

include(LimaTools)

# Set version
include(project_version)

# Enable python binding code compilation using sip generator
if (CAMERA_MASTER_PROJECT)
  option(LIMA_ENABLE_PYTHON "compile python binding code?" OFF)
  if (LIMA_ENABLE_PYTHON)
    limatools_find_python_and_sip()
  endif()
endif()

find_package(Espia REQUIRED)

file(GLOB_RECURSE ESPIA_INCS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(espia SHARED
  src/Espia.cpp
  src/FoclaHwInterface.cpp
  src/EspiaAcq.cpp
  src/EspiaBufferMgr.cpp
  src/EspiaDev.cpp
  src/EspiaFocla.cpp
  src/EspiaSerialLine.cpp
  ${ESPIA_INCS}
)

# Generate export macros
generate_export_header(espia)

# Add dependencies
target_include_directories(espia
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"           # For export header
  PUBLIC "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

# Set version and output name
set_target_properties(espia PROPERTIES
  OUTPUT_NAME "lima${PROJECT_NAME_LOWER}"
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

target_link_libraries(espia PUBLIC limacore)

target_compile_definitions(espia PUBLIC ${ESPIA_DEFINITIONS})

target_include_directories(espia PUBLIC $<BUILD_INTERFACE:${ESPIA_INCLUDE_DIRS}>)

target_link_libraries(espia PUBLIC ${ESPIA_LIBRARIES})

# Binding code for python
if(LIMA_ENABLE_PYTHON)
  limatools_run_sip_for_camera(espia)
endif()

# Generate and install package config file and version
if(CAMERA_MASTER_PROJECT)
  set(PROJECT_LIBRARIES espia)
  set(SIP_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/sip/lima)
  set(CMAKE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/cmake/lima)
  include(package_config)
endif()

## Installation

install(
  TARGETS espia
  EXPORT "${TARGETS_EXPORT_NAME}"
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # import library
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so files are libraries
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll files are binaries
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}  # this does not actually install anything (but used by downstream projects)
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  COMPONENT devel
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/project_version.h
    ${PROJECT_BINARY_DIR}/espia_export.h
  COMPONENT devel
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(LIMA_ENABLE_PYTHON)
  file(GLOB SIP_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/sip/*.sip"
  )

  install(
    FILES ${SIP_SOURCES}
          ${CMAKE_CURRENT_BINARY_DIR}/sip/limaespia.sip
    COMPONENT devel
    DESTINATION ${SIP_INSTALL_DIR}
  )

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/
    DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Espia"
  )
endif()

## Tests
if(CAMERA_ENABLE_TESTS)
  enable_testing()
  #add_subdirectory(test)
endif()
