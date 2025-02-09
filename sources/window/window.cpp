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
	m_renderer.reset(new Renderer{ m_window.get() });
}

void Window::update()
{
	glfwPollEvents();
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

Renderer& Window::getRenderer()
{
	return *m_renderer;
}
