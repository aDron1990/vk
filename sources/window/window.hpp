#pragma once

#include "window/input.hpp"
#include "graphics/vulkan/renderer.hpp"

#include <GLFW/glfw3.h>

class Window
{
public:
	Window(int widht, int height, std::string_view title);
	bool shouldClose();
	Renderer& getRenderer();
	Input& getInput();
	GLFWwindow* getWindow();
	
private:
	using HandlePtr = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)>;
	using RendererPtr = std::unique_ptr<Renderer>;
	using InputPtr = std::unique_ptr<Input>;

	HandlePtr m_window;
	RendererPtr m_renderer;
	InputPtr m_input;
	
};

