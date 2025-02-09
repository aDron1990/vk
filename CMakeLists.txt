cmake_minimum_required(VERSION 3.20)

project(vk)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

add_executable(vk 
	"sources/main.cpp"
	"sources/load_file.hpp"

	"sources/window/window.hpp"
	"sources/window/window.cpp"
	"sources/window/input.hpp"
	"sources/window/input.cpp"

	"sources/renderer/config.hpp"
	"sources/renderer/types.hpp"
	"sources/renderer/types.cpp"
	"sources/renderer/renderer.hpp"
	"sources/renderer/renderer.cpp"
	"sources/renderer/context.hpp"
	"sources/renderer/context.cpp"
	"sources/renderer/device.hpp"
	"sources/renderer/device.cpp"
	"sources/renderer/swapchain.hpp"
	"sources/renderer/swapchain.cpp"
	"sources/renderer/pipeline.hpp"
	"sources/renderer/pipeline.cpp"
	"sources/renderer/graphics_pipeline.hpp"
	"sources/renderer/graphics_pipeline.cpp"
	"sources/renderer/buffer.hpp"
	"sources/renderer/buffer.cpp"
	"sources/renderer/uniform_buffer.hpp"
	"sources/renderer/texture.hpp"
	"sources/renderer/texture.cpp"
	"sources/renderer/mesh.hpp"
	"sources/renderer/mesh.cpp"
	"sources/renderer/model.hpp"
	"sources/renderer/model.cpp"
	"sources/renderer/descriptor_pool.hpp"
	"sources/renderer/descriptor_pool.cpp"
	"sources/renderer/camera.hpp"
	"sources/renderer/camera.cpp"
	"sources/renderer/object.hpp"
	"sources/renderer/object.cpp"
)

set_property(TARGET vk PROPERTY CXX_STANDARD 23)
set_property(TARGET vk PROPERTY CXX_STANDARD_REQUIRED true)

target_include_directories(vk PRIVATE "sources/" ${Stb_INCLUDE_DIR})

target_link_libraries(vk PRIVATE
	glfw
	Vulkan::Vulkan
	glm::glm-header-only
	tinyobjloader::tinyobjloader
)