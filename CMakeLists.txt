# Copyright 2019-2021 Nicholas Corgan
#
# This file is part of SoapyVOLKConverters
#
# SoapyVOLKConverters is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# SoapyVOLKConverters is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SoapyVOLKConverters; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

# TODO: old-style VOLK includes and library links

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.1.0)
project(SoapyVOLKConverters CXX)
enable_testing()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

########################################################################
# Dependencies
########################################################################
find_package(SoapySDR "0.8" REQUIRED)
find_package(Volk REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SoapySDR_INCLUDE_DIRS}
    ${Volk_INCLUDE_DIRS})

########################################################################
# Build a Soapy module to add VOLK converters
########################################################################
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.in.h
    ${CMAKE_CURRENT_BINARY_DIR}/config.h)

SOAPY_SDR_MODULE_UTIL(
    TARGET volkConverters
    SOURCES SoapyVOLKConverters.cpp
    LIBRARIES
        Volk::volk
)

########################################################################
# Common code for unit test and benchmark
########################################################################
add_library(TestUtility STATIC TestUtility.cpp)
target_link_libraries(TestUtility Volk::volk)

########################################################################
# Add test
########################################################################
add_executable(TestSoapyVOLKConverters TestSoapyVOLKConverters.cpp)
add_test(TestSoapyVOLKConverters TestSoapyVOLKConverters)

# Link against Soapy, not the module, which is loaded at runtime
target_link_libraries(TestSoapyVOLKConverters
    TestUtility
    ${SoapySDR_LIBRARIES}
    Volk::volk)

########################################################################
# Benchmark VOLK vs. generic converters
########################################################################
add_executable(BenchmarkSoapyVOLKConverters BenchmarkSoapyVOLKConverters.cpp)

# Link against Soapy, not the module, which is loaded at runtime
target_link_libraries(BenchmarkSoapyVOLKConverters
    TestUtility
    ${SoapySDR_LIBRARIES}
    Volk::volk)

########################################################################
# Print Summary
########################################################################
MESSAGE(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
