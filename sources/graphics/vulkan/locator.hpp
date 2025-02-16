#pragma once

class Window;
class Renderer;
class Context;
class Device;
class Swapchain;

class Locator
{
public:
	static Window& getWindow();
	static Renderer& getRenderer();
	static Context& getContext();
	static Device& getDevice();
	static Swapchain& getSwapchain();

	static void setWindow(Window* window);
	static void setRenderer(Renderer* renderer);
	static void setContext(Context* context);
	static void setDevice(Device* device);
	static void setSwapchain(Swapchain* swapchain);

private:
	static Window* m_window;
	static Renderer* m_renderer;
	static Context* m_context;
	static Device* m_device;
	static Swapchain* m_swapchain;

};