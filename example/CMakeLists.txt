cmake_minimum_required(VERSION 3.31)

project(stay3-example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_example EXAMPLE_NAME SOURCE_FILE LIBS)
    add_executable(${EXAMPLE_NAME} ${SOURCE_FILE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE stay3 ${LIBS})
    if(EMSCRIPTEN)
        set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX ".html")
    endif()
endfunction()

add_example(app app.example.cpp "")
add_example(input input.example.cpp "")

add_subdirectory(dino)
