#pragma once

#include <string_view>
#include <memory>
#include <cstring>

#include "renderer/renderer.hpp"

class Window
{
public:
	virtual ~Window() = default;
	virtual Renderer& getRenderer() = 0;
	virtual void update() = 0;
	virtual bool shouldClose() = 0;
};

using WindowPtr = std::unique_ptr<Window>;
WindowPtr createWindow(uint_fast32_t width, uint_fast32_t height, std::string_view title);