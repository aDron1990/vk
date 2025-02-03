cmake_minimum_required(VERSION 3.20)

project(vk)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(vk 
	"sources/main.cpp"
	"sources/load_file.hpp"

	"sources/window/window.hpp"
	"sources/window/glfw_window.hpp"
	"sources/window/glfw_window.cpp"

	"sources/renderer/types.hpp"
	"sources/renderer/types.cpp"
	"sources/renderer/renderer.hpp"
	"sources/renderer/renderer.cpp"
	"sources/renderer/swapchain.hpp"
	"sources/renderer/swapchain.cpp"
)

set_property(TARGET vk PROPERTY CXX_STANDARD 23)
set_property(TARGET vk PROPERTY CXX_STANDARD_REQUIRED true)

target_include_directories(vk PRIVATE "sources/")

target_link_libraries(vk PRIVATE
	glfw
	Vulkan::Vulkan
	glm::glm-header-only
)