if(NOT EMSCRIPTEN)
    add_subdirectory(glfw)
    set(DAWN_FETCH_DEPENDENCIES ON)
    add_subdirectory(dawn)
else()
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

add_subdirectory(glm)
target_compile_definitions(
    glm
    PUBLIC GLM_ENABLE_EXPERIMENTAL
    PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE
    PUBLIC GLM_FORCE_LEFT_HANDED
)

add_subdirectory(entt)

add_subdirectory(stb)
