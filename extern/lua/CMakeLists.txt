cmake_minimum_required(VERSION 3.31)
project(lua LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "./lua/*.c" "./lua/*.h" "./lua.hpp")
list(FILTER LUA_SOURCES EXCLUDE REGEX "onelua\\.c$")

add_library(lua ${LUA_SOURCES})
set_target_properties(lua PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(lua SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
