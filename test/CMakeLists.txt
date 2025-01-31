cmake_minimum_required(VERSION 3.31)

project(stay3-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Catch2)

include(CTest)

function(add_custom_test TEST_NAME SOURCE_FILE LIBS ARGS)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE stay3 ${LIBS})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${ARGS})
endfunction()

add_custom_test(core-time core/time.test.cpp "Catch2::Catch2WithMain" "")
