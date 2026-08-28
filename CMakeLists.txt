cmake_minimum_required(VERSION 3.20)
project(ui_framework VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UI Framework is an SDL3-based library. SDL is provided by the consuming
# project; this repository does not vendor SDL sources or runtime binaries.
# The current minimum SDL3 API baseline is 3.2.0.
find_package(SDL3 3.2.0 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

add_library(ui_framework STATIC
    src/components/button.cpp
    src/components/checkbox.cpp
    src/components/dropdown.cpp
    src/components/image.cpp
    src/components/menu.cpp
    src/components/menu_item.cpp
    src/components/radio_button.cpp
    src/components/slider.cpp
    src/components/tab_control.cpp
    src/components/tab_item.cpp
    src/components/text_input.cpp
    src/components/toggle_button.cpp
    src/components/typography.cpp

    src/core/ui_manager.cpp
    src/core/node_tree.cpp
    src/core/node.cpp
    src/core/panel_node.cpp
    src/core/stack_panel_node.cpp
    src/core/text_layout.cpp
    src/core/text_content.cpp
    src/core/text_renderer.cpp
    src/core/text_edit_state.cpp
    src/core/layout_system.cpp
    src/core/modal_system.cpp
    src/core/input_system.cpp
    src/core/primitives.cpp
    src/core/layout_constraints.cpp
    src/core/linear_layout.cpp
    src/core/scroll_system.cpp
    src/core/animation_system.cpp
)

add_library(ui_framework::ui_framework ALIAS ui_framework)

target_include_directories(ui_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(ui_framework PUBLIC cxx_std_20)

target_link_libraries(ui_framework
    PUBLIC
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
)

option(UI_FRAMEWORK_BUILD_TESTS "Build ui_framework tests" OFF)

if(UI_FRAMEWORK_BUILD_TESTS)
    enable_testing()

    add_executable(ui_framework_animation_tests tests/core/animation_tests.cpp)
    add_executable(ui_framework_animation_property_api_tests tests/core/animation_property_api_tests.cpp)
    add_executable(ui_framework_input_tests tests/core/input_system_tests.cpp)
    add_executable(ui_framework_input_mutation_tests tests/core/input_system_mutation_tests.cpp)
    add_executable(ui_framework_node_tree_tests tests/core/node_tree_tests.cpp)
    add_executable(ui_framework_node_tree_regression_tests tests/core/node_tree_regression_tests.cpp)
    add_executable(ui_framework_ui_manager_regression_tests tests/core/ui_manager_regression_tests.cpp)
    add_executable(ui_framework_modal_system_regression_tests tests/core/modal_system_regression_tests.cpp)
    add_executable(ui_framework_scroll_system_regression_tests tests/core/scroll_system_regression_tests.cpp)
    add_executable(ui_framework_scroll_system_behavior_tests tests/core/scroll_system_behavior_tests.cpp)
    add_executable(ui_framework_layout_system_regression_tests tests/core/layout_system_regression_tests.cpp)
    add_executable(ui_framework_text_edit_state_tests tests/core/text_edit_state_tests.cpp)
    add_executable(ui_framework_text_input_tests tests/core/text_input_tests.cpp)
    add_executable(ui_framework_text_input_pointer_capture_tests tests/core/text_input_pointer_capture_tests.cpp)
    add_executable(ui_framework_text_input_submission_tests tests/core/text_input_submission_tests.cpp)
    add_executable(ui_framework_image_tests tests/core/image_tests.cpp)
    add_executable(ui_framework_event_system_behavior_tests tests/core/event_system_behavior_tests.cpp)

    set(UI_FRAMEWORK_TEST_TARGETS
        ui_framework_animation_tests
        ui_framework_animation_property_api_tests
        ui_framework_input_tests
        ui_framework_input_mutation_tests
        ui_framework_node_tree_tests
        ui_framework_node_tree_regression_tests
        ui_framework_ui_manager_regression_tests
        ui_framework_modal_system_regression_tests
        ui_framework_scroll_system_regression_tests
        ui_framework_scroll_system_behavior_tests
        ui_framework_layout_system_regression_tests
        ui_framework_text_edit_state_tests
        ui_framework_text_input_tests
        ui_framework_text_input_pointer_capture_tests
        ui_framework_text_input_submission_tests
        ui_framework_image_tests
        ui_framework_event_system_behavior_tests
    )

    foreach(TEST_TARGET IN LISTS UI_FRAMEWORK_TEST_TARGETS)
        target_include_directories(${TEST_TARGET}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        )

        target_link_libraries(${TEST_TARGET}
            PRIVATE
                ui_framework
        )

        add_test(
            NAME ${TEST_TARGET}
            COMMAND ${TEST_TARGET}
        )
    endforeach()
endif()
