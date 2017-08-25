cmake_minimum_required(VERSION 2.8)
project(ecce C CXX Fortran Java)

# TODO: Specify ECCE version here instead of data/client/config/Version
file(READ data/client/config/Version ECCE_VERSION)
string(STRIP ${ECCE_VERSION} ECCE_VERSION)
message("Generating Files for ECCE ${ECCE_VERSION}")

# CMake Utilities
include(ExternalProject)

set(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}/apps/wxpython ${CMAKE_PREFIX_PATH})
# set(wxWidgets_USE_UNICODE OFF)
# set(wxWidgets_CONFIG_OPTIONS --prefix=${CMAKE_INSTALL_PREFIX}/apps/wxpython)

# External Libraries
# find_package(wxWidgets REQUIRED)
find_package(GTK2 2.8 COMPONENTS gtk REQUIRED)
find_package(Freetype)
find_package(X11)
# include_directories(${X11_INCLUDE_DIR})
# find_package(Java REQUIRED)
# find_package(XercesC)

# Initialize Third Party Packages
# message(${wxWidgets_USE_FILE})
# include(${wxWidgets_USE_FILE})


# Set global variables
set(CMAKE_INC cmake)
set(ECCE_APPS apps)
set(ECCE_SERVER server)


# Setup C build environment
include_directories(include)

# Include Specific Rules
include(${CMAKE_INC}/3rdparty.cmake)
include(${CMAKE_INC}/lib.cmake)
include(${CMAKE_INC}/bin.cmake)
include(${CMAKE_INC}/fortran.cmake)
include(${CMAKE_INC}/java.cmake)
include(${CMAKE_INC}/static.cmake)
