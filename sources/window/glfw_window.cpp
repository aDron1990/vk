#include "window/glfw_window.hpp"

#include <stdexcept>
#include <cassert>

WindowPtr createWindow(uint_fast32_t width, uint_fast32_t height, std::string_view title)
{
	static bool created = false;
	assert(!created);
	created = true;
	return std::make_unique<GlfwWindow>(static_cast<int>(width), static_cast<int>(height), title);
}

GlfwWindow::GlfwWindow(int widht, int height, std::string_view title) : m_window{ nullptr, nullptr }
{
	if(!glfwInit())
		throw std::runtime_error{ "Failed to init GLFW" };

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto* window = glfwCreateWindow(widht, height, title.data(), nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error{ "Failed to create window" };

	m_window.reset(window);
	m_window.get_deleter() = [](GLFWwindow* window)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	};
	
	m_renderer.reset(new Renderer{m_window.get()});
}

Renderer& GlfwWindow::getRenderer()
{
	return *m_renderer;
}

void GlfwWindow::update()
{
	glfwPollEvents();
}

bool GlfwWindow::shouldClose()
{
	return glfwWindowShouldClose(m_window.get());
}