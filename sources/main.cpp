#include "window/window.hpp"

#include <GLFW/glfw3.h>

#include <print>

#define TRACY_ENABLE
#include <tracy/Tracy.hpp>

int main()
{
	try
	{
		auto window = Window{ 1280, 720, "window" };
		auto& renderer = window.getRenderer();
		auto& input = window.getInput();

		while (!window.shouldClose())
		{
			ZoneScopedN("main loop");
			input.update();
			renderer.render();
			if (input.getKey(GLFW_KEY_ESCAPE)) break;
		}
	}
	catch (std::exception& ex)
	{
		std::println("{}", ex.what());
	}
	return 0;
}