cmake_minimum_required(VERSION 3.7)
project(vkbench)

set(CMAKE_VERBOSE_MAKEFILE:BOOL ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

find_package(Vulkan REQUIRED)

include_directories(.)
include_directories(${Vulkan_INCLUDE_DIRS})
link_libraries(${Vulkan_LIBRARIES})

add_executable(vkbench src/main.cc src/utils.cc src/testBase.cc src/testUtil.cc
        src/submitTest.cc src/submitTest.h)
