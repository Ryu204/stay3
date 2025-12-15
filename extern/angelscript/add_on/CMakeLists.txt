cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(angelscript-with-addons LANGUAGES CXX)

set(SOURCE_FILES
    ../angelscript/sdk/add_on/scriptstdstring/scriptstdstring.h
    ../angelscript/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp
    ../angelscript/sdk/add_on/scriptstdstring/scriptstdstring.cpp

    ../angelscript/sdk/add_on/scriptarray/scriptarray.h
    ../angelscript/sdk/add_on/scriptarray/scriptarray.cpp

    ../angelscript/sdk/add_on/scriptbuilder/scriptbuilder.h
    ../angelscript/sdk/add_on/scriptbuilder/scriptbuilder.cpp

    ../angelscript/sdk/add_on/scriptdictionary/scriptdictionary.h
    ../angelscript/sdk/add_on/scriptdictionary/scriptdictionary.cpp
)

cmake_path(APPEND INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ../angelscript/sdk/add_on)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC angelscript)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE "${INCLUDE_PATH}")
