#include "window/window.hpp"

#include <GLFW/glfw3.h>

#include <print>

int main()
{
	try
	{
		auto window = Window{ 800, 600, "window" };
		auto& renderer = window.getRenderer();
		auto& input = window.getInput();

		while (!window.shouldClose())
		{
			window.update();
			renderer.draw();
		}
	}
	catch (std::exception& ex)
	{
		std::println("{}", ex.what());
	}
	return 0;
}