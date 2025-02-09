#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <unordered_map>
#include <array>

class Window;
class Input
{
public:
	Input(Window& window);
	bool getKey(uint32_t keyCode);
	void setKey(uint32_t keyCode, bool state);

private:
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
	Window& m_window;
	std::array<bool, 256> m_keyStates;
};

