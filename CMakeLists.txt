cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(stay3 VERSION 0.1 LANGUAGES CXX)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(extern)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PUBLIC FILE_SET my_modules_source TYPE CXX_MODULES FILES
    src/main.cppm
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dawn::webgpu_dawn
)

if (${PROJECT_NAME}_BUILD_TESTS)
    add_subdirectory(test)
endif()
