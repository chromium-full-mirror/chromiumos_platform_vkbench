cmake_minimum_required(VERSION 3.7)
project(vkbench)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

find_package(Vulkan REQUIRED)
find_package(PNG REQUIRED)
find_package(fmt REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${PNG_INCLUDE_DIRS})
link_libraries(${Vulkan_LIBRARIES})
link_libraries(${PNG_LIBRARIES})
link_libraries(fmt::fmt)

# Suppress deprecated warning
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")

# GLSL files to SPIR-V.
find_program(GLSL_VALIDATOR "glslangValidator")
if(NOT GLSL_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found!")
endif()
file(GLOB_RECURSE GLSL_SOURCE_FILES "src/shaders/*.frag" "src/shaders/*.vert")
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

file(GLOB_RECURSE TEST_FILES "src/tests/*.cc")
file(GLOB_RECURSE SRC_FILES "src/*.cc")
add_executable(vkbench ${SRC_FILES} ${SRC_FILES})
add_dependencies(vkbench Shaders)


set(INSTALL_DIR "/usr/local/graphics/${CMAKE_PROJECT_NAME}")
install(TARGETS vkbench
        RUNTIME DESTINATION ${INSTALL_DIR})
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders
        DESTINATION ${INSTALL_DIR}
        FILES_MATCHING PATTERN "*.spv")
