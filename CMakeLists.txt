cmake_minimum_required(VERSION 2.8)
project(ecce C CXX Fortran Java)

# CMake Utilities
include(ExternalProject)

# External Libraries
find_package(wxWidgets COMPONENTS core base REQUIRED)
find_package(GTK2 2.8 COMPONENTS gtk REQUIRED)
find_package(Freetype)
# find_package(Java REQUIRED)
# find_package(XercesC)
set(wxWidgets_USE_UNICODE OFF)

# Initialize Third Party Packages
include(${wxWidgets_USE_FILE})

# Set global variables
set(CMAKE_INC ${CMAKE_SOURCE_DIR}/cmake)

# Setup C build environment
include_directories(${CMAKE_SOURCE_DIR}/include)

# Include Specific Rules
include(${CMAKE_INC}/3rdparty.cmake)
include(${CMAKE_INC}/lib.cmake)
include(${CMAKE_INC}/bin.cmake)
include(${CMAKE_INC}/fortran.cmake)
include(${CMAKE_INC}/java.cmake)
include(${CMAKE_INC}/static.cmake)
