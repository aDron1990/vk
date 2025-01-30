#include "window/window.hpp"

#include <GLFW/glfw3.h>

#include <print>

int main()
{
	try
	{
		auto window = createWindow(800, 600, "window");

		while (!window->shouldClose())
		{
			window->update();
		}

	}
	catch (std::exception& ex)
	{
		std::println("{}", ex.what());
	}
	return 0;
}