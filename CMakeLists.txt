cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(stay3 VERSION 0.1 LANGUAGES CXX)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)
option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(extern)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET my_modules_source TYPE CXX_MODULES FILES

    src/main.cppm

    src/program/mod.cppm
    src/program/config.cppm
    src/program/app.cppm

    src/graphics/mod.cppm
    src/graphics/error.cppm
    src/graphics/vertex.cppm
    src/graphics/glfw_window.cppm
    src/graphics/camera.cppm
    src/graphics/material.cppm
    src/graphics/texture.cppm
    src/graphics/rendered_mesh.cppm
    src/graphics/mesh_builder.cppm
    src/graphics/font.cppm
    src/graphics/text.cppm

    src/core/mod.cppm
    src/core/error.cppm
    src/core/time.cppm
    src/core/vector.cppm
    src/core/quaternion.cppm
    src/core/transform.cppm
    src/core/matrix.cppm
    src/core/math_ops.cppm
    src/core/math.cppm
    src/core/id_generator.cppm
    src/core/signal.cppm
    src/core/logger.cppm
    src/core/file.cppm
    src/core/color.cppm
    src/core/variant_helper.cppm
    src/core/rect.cppm
    src/core/any_map.cppm

    src/input/mod.cppm
    src/input/event.cppm
    src/input/keyboard.cppm

    src/node/mod.cppm
    src/node/node.cppm
    src/node/tree_context.cppm

    src/ecs/mod.cppm
    src/ecs/entity.cppm
    src/ecs/ecs_registry.cppm
    src/ecs/entities_holder.cppm
    src/ecs/system_data.cppm
    src/ecs/system_wrapper.cppm
    src/ecs/system_manager.cppm
    src/ecs/component.cppm
    src/ecs/dependency.cppm
    src/ecs/component_ref.cppm

    src/systems/mod.cppm
    src/systems/runtime_info.cppm
    src/systems/transform/global_transform.cppm
    src/systems/render/mod.cppm
    src/systems/render/render_system.cppm
    src/systems/render/components.cppm
    src/systems/render/init_result.cppm
    src/systems/render/config.cppm
    src/systems/render/pipeline.cppm
    src/systems/render/bind_group_layouts.cppm
    src/systems/render/render_pass.cppm
    src/systems/render/material.cppm
    src/systems/render/texture_subsystem.cppm
    src/systems/render/material_subsystem.cppm
    src/systems/render/mesh_subsystem.cppm
    src/systems/text/text_system.cppm
    src/systems/text/font_state.cppm
    src/systems/text/font_atlas.cppm
    src/systems/text/text_state.cppm

    PRIVATE

    src/program/app.cpp

    src/graphics/glfw_window.cpp

    src/core/time.cpp
    src/core/transform.cpp

    src/node/node.cpp

    src/systems/transform/global_transform.cpp
    src/systems/render/init_result.cpp
    src/systems/render/pipeline.cpp
    src/systems/render/render_system.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glm EnTT::EnTT stb freetype)
if(EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PUBLIC -fexceptions)
    target_link_options(${PROJECT_NAME} PUBLIC -fexceptions -sASYNCIFY)
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3
        -sUSE_WEBGPU
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dawn::webgpu_dawn webgpu_glfw
    )
endif()

if(${PROJECT_NAME}_BUILD_TESTS)
    add_subdirectory(test)
endif()

if(${PROJECT_NAME}_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
