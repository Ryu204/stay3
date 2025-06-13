cmake_minimum_required(VERSION 3.31)

project(stay3-example-slope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(slope-example main.cpp)
if(EMSCRIPTEN)
    set_target_properties(slope-example PROPERTIES SUFFIX ".html")
endif()
target_link_libraries(slope-example PRIVATE stay3)
stay3_app_target(slope-example)
stay3_add_assets(slope-example "${CMAKE_CURRENT_SOURCE_DIR}/assets" "assets")
