if(NOT EMSCRIPTEN)
    add_subdirectory(glfw)
    set(DAWN_FETCH_DEPENDENCIES ON)
else()
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()

# Apparently dawn has support for webgpu emscripten version
add_subdirectory(dawn)

add_subdirectory(glm)
target_compile_definitions(
    glm
    PUBLIC GLM_ENABLE_EXPERIMENTAL
    PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE
    PUBLIC GLM_FORCE_LEFT_HANDED
)

add_subdirectory(entt)
target_compile_definitions(
    EnTT
    INTERFACE ENTT_NOEXCEPTION
)

add_subdirectory(stb)

add_subdirectory(freetype)

set(CPP_RTTI_ENABLED ON)
add_subdirectory(JoltPhysics/Build)

if(EMSCRIPTEN)
    set(WASM_EXCEPTION_DEPENDENCIES freetype)
    foreach(dep IN LISTS WASM_EXCEPTION_DEPENDENCIES)
        target_compile_options(${dep} PRIVATE -fwasm-exceptions)
        target_link_options(${dep} PRIVATE -fwasm-exceptions)
    endforeach()
endif()
