#pragma once

#include <entt/entt.hpp>

class Window;
class Renderer;
class Context;
class Device;
class Swapchain;
class DescriptorPool;
class TextureArray;
template<typename T>
class DUB;
struct Material;

class Locator
{
public:
	static Window& getWindow();
	static Renderer& getRenderer();
	static Context& getContext();
	static Device& getDevice();
	static Swapchain& getSwapchain();
	static DescriptorPool& getDescriptorPool();
	static TextureArray& getTextureArray();
	static entt::registry& getECS();
	static DUB<Material>& getMaterialBuffer();

	static void setWindow(Window* window);
	static void setRenderer(Renderer* renderer);
	static void setContext(Context* context);
	static void setDevice(Device* device);
	static void setSwapchain(Swapchain* swapchain);
	static void setDescriptorPool(DescriptorPool* descriptorPool);
	static void setTextureArray(TextureArray* textureArray);
	static void setECS(entt::registry* ecs);
	static void setMaterialBuffer(DUB<Material>* materialBuffer);

private:
	static Window* m_window;
	static Renderer* m_renderer;
	static Context* m_context;
	static Device* m_device;
	static Swapchain* m_swapchain;
	static DescriptorPool* m_descriptorPool;
	static TextureArray* m_textureArray;
	static entt::registry* m_ecs;
	static DUB<Material>* m_materialBuffer;
};