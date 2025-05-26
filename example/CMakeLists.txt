cmake_minimum_required(VERSION 3.31)

project(stay3-example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_example EXAMPLE_NAME SOURCE_FILE LIBS)
    set(EXAMPLE_NAME ${EXAMPLE_NAME}-example)
    add_executable(${EXAMPLE_NAME} ${SOURCE_FILE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE stay3 ${LIBS})
    stay3_app_target(${EXAMPLE_NAME})
    stay3_add_assets(${EXAMPLE_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/assets" "assets")
    if(EMSCRIPTEN)
        set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX ".html")
    endif()
endfunction()

add_example(app app.example.cpp "")
add_example(input input.example.cpp "")
add_example(text text.example.cpp "")
add_example(physics physics.example.cpp "")
add_example(earth earth.example.cpp "")

add_subdirectory(dino)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
