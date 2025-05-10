cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(stb LANGUAGES CXX)

add_library(
    ${PROJECT_NAME}
    stb/stb_image.h
    stb/stb_rect_pack.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
