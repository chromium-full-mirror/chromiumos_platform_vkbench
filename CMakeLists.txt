cmake_minimum_required(VERSION 3.7)
project(vkbench)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

find_package(Vulkan REQUIRED)

include_directories(.)
include_directories(${Vulkan_INCLUDE_DIRS})
link_libraries(${Vulkan_LIBRARIES})

# Suppress deprecated warning
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
add_executable(vkbench src/main.cc src/utils.cc src/vkBase.cc
        src/submitTest.cc)

install (
    FILES ${CMAKE_BINARY_DIR}/vkbench
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/bin
)
