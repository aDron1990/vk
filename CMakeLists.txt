cmake_minimum_required(VERSION 3.20)

project(vk)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(CMakeRC CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

add_executable(vk 
	"sources/main.cpp"

	"sources/window/window.hpp"
	"sources/window/window.cpp"
	"sources/window/input.hpp"
	"sources/window/input.cpp"

	"sources/graphics/vulkan/config.hpp"
	"sources/graphics/vulkan/types.hpp"
	"sources/graphics/vulkan/types.cpp"
	"sources/graphics/vulkan/renderer.hpp"
	"sources/graphics/vulkan/renderer.cpp"
	"sources/graphics/vulkan/pipeline.hpp"
	"sources/graphics/vulkan/pipeline.cpp"
	"sources/graphics/vulkan/buffer.hpp"
	"sources/graphics/vulkan/buffer.cpp"
	"sources/graphics/vulkan/uniform_buffer.hpp"
	"sources/graphics/vulkan/mesh.hpp"
	"sources/graphics/vulkan/mesh.cpp"
	"sources/graphics/vulkan/model.hpp"
	"sources/graphics/vulkan/model.cpp"
	"sources/graphics/vulkan/camera.hpp"
	"sources/graphics/vulkan/camera.cpp"
	"sources/graphics/vulkan/object.hpp"
	"sources/graphics/vulkan/object.cpp"
	"sources/graphics/vulkan/locator.hpp"
	"sources/graphics/vulkan/locator.cpp"

	"sources/graphics/vulkan/context/context.hpp"
	"sources/graphics/vulkan/context/context.cpp"
	"sources/graphics/vulkan/context/device.hpp"
	"sources/graphics/vulkan/context/device.cpp"
	"sources/graphics/vulkan/context/swapchain.hpp"
	"sources/graphics/vulkan/context/swapchain.cpp"

	"sources/graphics/vulkan/descriptor/descriptor_pool.hpp"
	"sources/graphics/vulkan/descriptor/descriptor_pool.cpp"
	"sources/graphics/vulkan/descriptor/descriptor_pool_props.hpp"

	"sources/graphics/vulkan/render_pass/render_pass.hpp"
	"sources/graphics/vulkan/render_pass/offscreen_pass.hpp"
	"sources/graphics/vulkan/render_pass/offscreen_pass.cpp"
	"sources/graphics/vulkan/render_pass/swapchain_pass.hpp"
	"sources/graphics/vulkan/render_pass/swapchain_pass.cpp"
	"sources/graphics/vulkan/render_pass/framebuffer_props.hpp"
	"sources/graphics/vulkan/render_pass/framebuffer.hpp"
	
	"sources/graphics/vulkan/render_pass/offscreen_framebuffer.hpp"
	"sources/graphics/vulkan/render_pass/offscreen_framebuffer.cpp"
	"sources/graphics/vulkan/render_pass/swapchain_framebuffer.hpp"
	"sources/graphics/vulkan/render_pass/swapchain_framebuffer.cpp"

	"sources/graphics/vulkan/image/image_view.hpp"
	"sources/graphics/vulkan/image/sampler.hpp"
	"sources/graphics/vulkan/image/texture.hpp"
	"sources/graphics/vulkan/image/image_texture.hpp"
	"sources/graphics/vulkan/image/image_texture.cpp"
	"sources/graphics/vulkan/image/render_texture.hpp"
	"sources/graphics/vulkan/image/render_texture.cpp"
	"sources/graphics/vulkan/image/swapchain_image.hpp"
	"sources/graphics/vulkan/image/swapchain_image.cpp"
)

set_property(TARGET vk PROPERTY CXX_STANDARD 23)
set_property(TARGET vk PROPERTY CXX_STANDARD_REQUIRED true)

target_include_directories(vk PRIVATE "sources/" ${Stb_INCLUDE_DIR})

target_link_libraries(vk PRIVATE
	glfw
	Vulkan::Vulkan
	glm::glm-header-only
	tinyobjloader::tinyobjloader
	imgui::imgui
	Tracy::TracyClient
)

# Shaders
include(cmake/add_shader.cmake)

set(shader_files
	"resources/shaders/main/shader.vert"
    "resources/shaders/main/shader.frag"
	"resources/shaders/blur/horizontal.vert"
    "resources/shaders/blur/horizontal.frag"
	"resources/shaders/blur/vertical.vert"
    "resources/shaders/blur/vertical.frag"
	"resources/shaders/emiter/shader.vert"
    "resources/shaders/emiter/shader.frag"
	"resources/shaders/pick_bright/shader.vert"
    "resources/shaders/pick_bright/shader.frag"
	"resources/shaders/combine/shader.vert"
    "resources/shaders/combine/shader.frag"
	"resources/shaders/shadow/shader.vert"
    "resources/shaders/shadow/shader.frag"
)

add_shader("${shader_files}" spv_names)
message(${spv_names})
add_custom_target(shader_dep ALL DEPENDS ${spv_names})
cmrc_add_resource_library(shaders ${spv_names})
add_dependencies(shaders shader_dep)

cmrc_add_resource_library(images
	"resources/images/statue.jpg"
	"resources/images/viking_room.png"
	"resources/images/container2.png"
    "resources/images/container2_specular.png"
    "resources/images/brown.png"
    "resources/images/brown_specular.png"
)

cmrc_add_resource_library(models
    "resources/models/monkey.obj"
	"resources/models/monkey.mtl"
	"resources/models/plane.obj"
	"resources/models/plane.mtl"
)

target_link_libraries(vk PRIVATE shaders images models)