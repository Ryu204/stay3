cmake_minimum_required(VERSION 3.31)

project(stay3-example-dino LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(dino main.cpp)
target_link_libraries(dino PRIVATE stay3)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
