cmake_minimum_required(VERSION 3.31)

project(stay3-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main-test main.test.cpp)

target_link_libraries(main-test PRIVATE stay3)