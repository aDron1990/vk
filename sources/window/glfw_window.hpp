#pragma once

#include "window/window.hpp"

#include <GLFW/glfw3.h>

class GlfwWindow : public Window
{
public:
	GlfwWindow(int widht, int height, std::string_view title);
	Renderer& getRenderer() override;
	void update() override;
	bool shouldClose() override;

private:
	using HandlePtr = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)>;
	using RendererPtr = std::unique_ptr<Renderer>;

	HandlePtr m_window;
	RendererPtr m_renderer;
};

