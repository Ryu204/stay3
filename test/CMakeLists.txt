cmake_minimum_required(VERSION 3.31)

project(stay3-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

function(add_custom_test TEST_NAME SOURCE_FILE LIBS ARGS)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE stay3 ${LIBS})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${ARGS})
endfunction()

add_custom_test(main-test main.test.cpp "" "")
