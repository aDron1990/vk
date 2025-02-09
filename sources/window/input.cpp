#include "window/input.hpp"
#include "window/window.hpp"

#include <iostream>

Input::Input(Window& window) : m_window{ window }, m_keyStates{}
{
	auto* gwindow = m_window.getWindow();
	glfwSetWindowUserPointer(gwindow, this);
	glfwSetKeyCallback(gwindow, keyCallback);
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT) return;
	auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
	input->setKey(key, action);
}

bool Input::getKey(uint32_t keyCode)
{
	return m_keyStates[keyCode];
}

void Input::setKey(uint32_t keyCode, bool state)
{
	m_keyStates[keyCode] = state;
}