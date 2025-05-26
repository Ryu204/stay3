cmake_minimum_required(VERSION 3.31)

project(stay3-test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Catch2)
if(EMSCRIPTEN)
    target_compile_options(Catch2 PUBLIC -fexceptions)
    target_link_options(Catch2 PUBLIC -fexceptions)
endif()


include(CTest)


add_library(test-helper)
target_link_libraries(test-helper PRIVATE stay3)
target_sources(test-helper
    PUBLIC FILE_SET my_modules_source TYPE CXX_MODULES FILES
    core/test_helper.cppm
)

function(add_custom_test TEST_NAME SOURCE_FILE LIBS ARGS)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE stay3 test-helper ${LIBS})
    stay3_app_target(${TEST_NAME})
    stay3_add_assets(${TEST_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/assets" assets)

    if(MSVC)
        target_compile_options(${TEST_NAME} PRIVATE /W4)
    elseif(EMSCRIPTEN)
        target_compile_options(${TEST_NAME} PRIVATE -g)
    else()
        target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(EMSCRIPTEN)
        set(TEST_SHELL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/web/test-shell.html)
        set_target_properties(${TEST_NAME} PROPERTIES SUFFIX ".html" LINK_DEPENDS ${TEST_SHELL_FILE})
        target_link_options(${TEST_NAME} PRIVATE
            --shell-file ${TEST_SHELL_FILE}
            -sEXIT_RUNTIME=1
        )
        add_test(NAME ${TEST_NAME} COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/web/index.js -d ${CMAKE_CURRENT_BINARY_DIR} -i ${TEST_NAME}.html ${ARGS})
    else()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${ARGS})
    endif()
endfunction()

add_custom_test(core-time core/time.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-vector core/vector.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-matrix core/matrix.test.cpp "Catch2::Catch2WithMain;glm" "")
add_custom_test(core-quaternion core/quaternion.test.cpp "Catch2::Catch2WithMain;glm" "")
add_custom_test(core-transform core/transform.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-id-generator core/id_generator.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-signal core/signal.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-file core/file.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-color core/color.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(core-any-map core/any_map.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(node-node node/node.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(node-node-ecs node/node_ecs.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(input-event input/event.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(input-keyboard input/keyboard.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(graphics-glfw-window graphics/glfw_window.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(graphics-mesh-builder graphics/mesh_builder.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(ecs-ecs-registry ecs/ecs_registry.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-ecs-registry-advanced ecs/ecs_registry_advanced.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-entities-holder ecs/entities_holder.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-system-data ecs/system_data.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-system-manager ecs/system_manager.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-dependency ecs/dependency.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-component-ref ecs/component_ref.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(ecs-entity ecs/entity.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(systems-global-transform systems/global_transform.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(systems-global-transform-advanced systems/global_transform_advanced.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(systems-render-camera systems/render_camera.test.cpp "Catch2::Catch2WithMain" "")
add_custom_test(systems-render-system systems/render_system.test.cpp "Catch2::Catch2WithMain" "")

add_custom_test(physics-world physics/world.test.cpp "Catch2::Catch2WithMain" "")
