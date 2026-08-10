cmake_minimum_required(VERSION 3.20)
project(EchoesEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fpermissive)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/W4 /fsanitize=address /Zi /Od)
    else()
        add_compile_options(-fsanitize=address,undefined -g -O0 -Wall -Wextra)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm CONFIG QUIET)

add_executable(eoa_engine
    src/main.cpp
    src/platform/window.cpp
    src/platform/camera.cpp
    src/rhi/vk_instance.cpp
    src/rhi/vk_device.cpp
    src/rhi/vk_swapchain.cpp
    src/rhi/file_utils.cpp
    src/renderer/renderer.cpp
    src/renderer/pipeline.cpp
    src/renderer/buffer_utils.cpp
    src/renderer/mesh.cpp
    src/renderer/texture.cpp
    src/renderer/gltf_loader.cpp
    src/renderer/particle_system.cpp
    src/core/object.cpp
    src/core/component.cpp
    src/core/actor.cpp
    src/core/transform_component.cpp
    src/core/serializer.cpp
    src/renderer/mesh_component.cpp
    src/renderer/gbuffer.cpp
    src/renderer/deferred_lighting.cpp
    src/renderer/shadow_pass.cpp
    src/editor/editor.cpp
    src/vendor/stb_image_impl.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_impl_vulkan.cpp
    third_party/imgui/imgui_impl_glfw.cpp
    third_party/imguizmo/src/ImGuizmo.cpp
)

target_include_directories(eoa_engine PRIVATE src third_party third_party/imgui third_party/imguizmo/src)
target_link_libraries(eoa_engine PRIVATE Vulkan::Vulkan glfw)
if(TARGET glm::glm)
    target_link_libraries(eoa_engine PRIVATE glm::glm)
endif()

find_program(GLSLC glslc REQUIRED)
file(GLOB SHADER_SOURCES ${CMAKE_SOURCE_DIR}/shaders/*.vert ${CMAKE_SOURCE_DIR}/shaders/*.frag)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(eoa_engine shaders)

add_custom_command(TARGET eoa_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
)

if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    get_filename_component(MSVC_TOOLS_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    add_custom_command(TARGET eoa_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MSVC_TOOLS_BIN_DIR}/clang_rt.asan_dynamic-x86_64.dll"
            $<TARGET_FILE_DIR:eoa_engine>
    )
endif()
