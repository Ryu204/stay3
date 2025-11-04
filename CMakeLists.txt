cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(stay3 VERSION 0.1 LANGUAGES CXX)

option(stay3_BUILD_TESTS "Build tests" OFF)
option(stay3_BUILD_EXAMPLES "Build examples" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(extern)

add_library(stay3)

target_sources(stay3
    PUBLIC FILE_SET my_modules_source TYPE CXX_MODULES FILES

    src/main.cppm

    src/program/mod.cppm
    src/program/config.cppm
    src/program/app.cppm
    src/program/app_launcher.cppm

    src/graphics/core/mod.cppm
    src/graphics/core/error.cppm
    src/graphics/core/vertex.cppm
    src/graphics/core/glfw_window.cppm
    src/graphics/core/camera.cppm
    src/graphics/core/material.cppm
    src/graphics/core/texture.cppm
    src/graphics/core/rendered_mesh.cppm
    src/graphics/core/mesh_builder.cppm
    src/graphics/text/mod.cppm
    src/graphics/text/font.cppm
    src/graphics/text/text.cppm
    src/graphics/mod.cppm

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

    src/physics/physics_debug.cppm

    src/physics/mod.cppm
    src/physics/world.cppm
    src/physics/contact_listener.cppm
    src/physics/rigidbody.cppm
    src/physics/properties.cppm
    src/physics/collider.cppm
    src/physics/convert.cppm

    src/systems/mod.cppm
    src/systems/runtime_info.cppm

    src/systems/transform/global_transform.cppm

    src/systems/render/priv/mod.cppm
    src/systems/render/render_system.cppm
    src/systems/render/priv/components.cppm
    src/systems/render/priv/init_result.cppm
    src/systems/render/config.cppm
    src/systems/render/priv/pipeline.cppm
    src/systems/render/priv/bind_group_layouts.cppm
    src/systems/render/priv/render_pass.cppm
    src/systems/render/priv/material.cppm
    src/systems/render/priv/texture_subsystem.cppm
    src/systems/render/priv/material_subsystem.cppm
    src/systems/render/priv/mesh_subsystem.cppm
    src/systems/render/priv/wait.cppm

    src/systems/text/text_system.cppm
    src/systems/text/priv/mod.cppm
    src/systems/text/priv/font_state.cppm
    src/systems/text/priv/font_atlas.cppm
    src/systems/text/priv/text_state.cppm

    src/systems/physics/physics_system.cppm

    src/systems/scripts/script_system.cppm
    src/systems/scripts/script_component.cppm
    src/systems/scripts/script_database.cppm
    src/systems/scripts/error.cppm
    src/systems/scripts/script_langs.cppm

    src/systems/scripts/lua/lua_system.cppm

    PRIVATE

    src/program/app.cpp

    src/graphics/core/glfw_window.cpp

    src/core/time.cpp
    src/core/transform.cpp

    src/node/node.cpp

    src/systems/transform/global_transform.cpp
    src/systems/render/priv/init_result.cpp
    src/systems/render/priv/pipeline.cpp
    src/systems/render/render_system.cpp
)

target_link_libraries(stay3 PRIVATE glfw webgpu_glfw glm EnTT::EnTT stb freetype lua sol2::sol2)
# Targets linked to Jolt must compile with same instruction-related flags (avx, bmi,...)
target_link_libraries(stay3 PUBLIC Jolt)
if(EMSCRIPTEN)
    target_compile_options(stay3 PUBLIC -fwasm-exceptions)
    target_link_options(stay3 PUBLIC -fwasm-exceptions -sJSPI -sALLOW_MEMORY_GROWTH)
    target_link_options(stay3 PRIVATE -sUSE_GLFW=3)
    target_link_libraries(stay3 PRIVATE dawn::emdawnwebgpu_cpp)
else()
    target_link_libraries(stay3 PRIVATE dawn::webgpu_dawn)
endif()

# Copy SOURCE_DIR to new place so its relative path from TARGET executable will be DESTINATION_DIR
function(stay3_add_assets TARGET SOURCE_DIR DESTINATION_DIR)
    if(EMSCRIPTEN)
        target_link_options(${TARGET} PRIVATE --preload-file=${SOURCE_DIR}@${DESTINATION_DIR})
    else()
        # Generate list of files to copy
        file(GLOB_RECURSE SOURCE_FILES RELATIVE "${SOURCE_DIR}" CONFIGURE_DEPENDS "${SOURCE_DIR}/*")
        foreach(FILE IN LISTS SOURCE_FILES)
            set(SRC "${SOURCE_DIR}/${FILE}")
            set(DST "$<TARGET_FILE_DIR:${TARGET}>/${DESTINATION_DIR}/${FILE}")
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC}" "${DST}"
                COMMENT "Copying asset: ${SRC} -> ${DST}")
        endforeach()
    endif()
endfunction()

# Adds extra configuration for application using stay3
function(stay3_app_target TARGET)
    stay3_add_assets(${TARGET} "${CMAKE_SOURCE_DIR}/assets" "assets/stay3")
endfunction()

if(stay3_BUILD_TESTS)
    add_subdirectory(test)
endif()

if(stay3_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
