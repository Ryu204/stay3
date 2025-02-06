if (NOT EMSCRIPTEN)
    set(DAWN_FETCH_DEPENDENCIES ON)
    add_subdirectory(dawn)
endif()
