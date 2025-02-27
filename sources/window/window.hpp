#pragma once

#include "window/input.hpp"
#include "graphics/vulkan/render_system.hpp"

#include <GLFW/glfw3.h>

class Window
{
public:
	Window(int widht, int height, std::string_view title);
	bool shouldClose();
	RenderSystem& getRenderSystem();
	Input& getInput();
	GLFWwindow* getWindow();
	
private:
	using HandlePtr = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)>;
	using RenderSystemPtr = std::unique_ptr<RenderSystem>;
	using InputPtr = std::unique_ptr<Input>;

	HandlePtr m_window;
	RenderSystemPtr m_renderSystem;
	InputPtr m_input;
	
};

