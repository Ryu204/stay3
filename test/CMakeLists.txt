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
    if(EMSCRIPTEN)
        set(TEST_SHELL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/web/test-shell.html)
        set_target_properties(${TEST_NAME} PROPERTIES SUFFIX ".html" LINK_DEPENDS ${TEST_SHELL_FILE})
        target_link_options(${TEST_NAME} PRIVATE --shell-file ${TEST_SHELL_FILE} -s EXIT_RUNTIME=1)
        add_test(NAME ${TEST_NAME} COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/web/index.js -d ${CMAKE_CURRENT_BINARY_DIR} -i ${TEST_NAME}.html ${ARGS})
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${ARGS})
    endif()
endfunction()

add_custom_test(system-time system/time.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(system-vector system/vector.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(window-event window/event.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(graphics-glfw-window graphics/glfw_window.test.cpp "Catch2::Catch2WithMain" "")
