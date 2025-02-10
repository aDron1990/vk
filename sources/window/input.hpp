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
	void update();
	bool getCursorLock();
	void lockCursor(bool lock = true);

	bool getKey(uint32_t keyCode);
	bool getKeyDown(uint32_t keyCode);
	bool getKeyUp(uint32_t keyCode);
	void setKey(uint32_t keyCode, bool state);

	glm::dvec2 getCursorPos();
	glm::dvec2 getCursorDelta();
	void setCursorPos(glm::dvec2 cursorPos);

private:
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

private:
	Window& m_window;
	std::array<bool, 1024> m_keyStates{};
	std::array<bool, 1024> m_keyDownStates{};
	std::array<bool, 1024> m_keyUpStates{};
	glm::dvec2 m_lastCursorPos;
	glm::dvec2 m_cursorDelta{};
	bool m_cursorLock{false};
};

