# Set projectname (must be done AFTER setting configurationtypes)
project(ProjectSecureChat)

# CMake policies (can not be handled elsewhere)
cmake_minimum_required(VERSION 2.6)
#cmake_policy(SET CMP0005 OLD)

# Set RPATH-handing (CMake parameters)
set(CMAKE_SKIP_BUILD_RPATH 0)
set(CMAKE_BUILD_WITH_INSTALL_RPATH 0)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH 1)

# set macro-directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

# build in Release-mode by default if not explicitly set
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "Release")
endif()

include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

# set default buildoptions and print them
include(cmake/options.cmake)

include(CheckPlatform)

# basic packagesearching and setup (further support will be needed, this is a preliminary release!)
find_package(OpenSSL REQUIRED)
if( CLIENT )
    find_package(GTK REQUIRED)
endif()

# Find revision ID and hash of the sourcetree
#include(cmake/genrev.cmake)

# print out the results before continuing
include(cmake/showoptions.cmake)

# add core sources
add_subdirectory(src)