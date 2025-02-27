#include "window/window.hpp"

#include <stdexcept>
#include <cassert>

Window::Window(int widht, int height, std::string_view title) : m_window{ nullptr, nullptr }
{
	if(!glfwInit())
		throw std::runtime_error{ "Failed to init GLFW" };

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(widht, height, title.data(), nullptr, nullptr);
	if (window == nullptr)
		throw std::runtime_error{ "Failed to create window" };

	m_window.reset(window);
	m_window.get_deleter() = [](GLFWwindow* window)
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	};
	
	m_input.reset(new Input{ *this });
	m_renderSystem.reset(new RenderSystem{ *this });
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(m_window.get());
}

GLFWwindow* Window::getWindow()
{
	return m_window.get();
}

Input& Window::getInput()
{
	return *m_input;
}

RenderSystem& Window::getRenderSystem()
{
	return *m_renderSystem;
}
