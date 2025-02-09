#include "window/input.hpp"
#include "window/window.hpp"

#include <print>

Input::Input(Window& window) : m_window{ window }, m_keyStates{}
{
	auto* gwindow = m_window.getWindow();
	glfwSetWindowUserPointer(gwindow, this);
	glfwSetKeyCallback(gwindow, keyCallback);
	glfwGetCursorPos(gwindow, &m_lastCursorPos.x, &m_lastCursorPos.y);
	glfwSetCursorPosCallback(gwindow, cursorPosCallback);
	glfwSetInputMode(gwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT) return;
	auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
	input->setKey(key, action);
}

void Input::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
	input->setCursorPos({ xpos, ypos });
}

bool Input::getKey(uint32_t keyCode)
{
	return m_keyStates[keyCode];
}

void Input::setKey(uint32_t keyCode, bool state)
{
	m_keyStates[keyCode] = state;
}

glm::dvec2 Input::getCursorPos()
{
	return m_lastCursorPos;
}

glm::dvec2 Input::getCursorDelta()
{
	glm::dvec2 temp{};
	std::swap(temp, m_cursorDelta);
	return temp;
}

void Input::setCursorPos(glm::dvec2 cursorPos)
{
	m_cursorDelta = cursorPos - m_lastCursorPos;
	m_lastCursorPos = cursorPos;
}
