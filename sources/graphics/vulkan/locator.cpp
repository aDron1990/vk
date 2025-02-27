#include "graphics/vulkan/locator.hpp"

#include <cassert>

Window* Locator::m_window = nullptr;
Renderer* Locator::m_renderer = nullptr;
Context* Locator::m_context = nullptr;
Device* Locator::m_device = nullptr;
Swapchain* Locator::m_swapchain = nullptr;
DescriptorPool* Locator::m_descriptorPool = nullptr;
TextureArray* Locator::m_textureArray = nullptr;
entt::registry* Locator::m_ecs = nullptr;
DUB<Material>* Locator::m_materialBuffer = nullptr;

Window& Locator::getWindow()
{
	assert(m_window != nullptr);
	return *m_window;
}

Renderer& Locator::getRenderer()
{
	assert(m_renderer != nullptr);
	return *m_renderer;
}

Context& Locator::getContext()
{
	assert(m_context != nullptr);
	return *m_context;
}

Device& Locator::getDevice()
{
	assert(m_device != nullptr);
	return *m_device;
}

Swapchain& Locator::getSwapchain()
{
	assert(m_swapchain != nullptr);
	return *m_swapchain;
}

DescriptorPool& Locator::getDescriptorPool()
{
	assert(m_descriptorPool != nullptr);
	return *m_descriptorPool;
}

TextureArray& Locator::getTextureArray()
{
	assert(m_textureArray != nullptr);
	return *m_textureArray;
}

entt::registry& Locator::getECS()
{
	assert(m_ecs != nullptr);
	return *m_ecs;
}

DUB<Material>& Locator::getMaterialBuffer()
{
	assert(m_materialBuffer != nullptr);
	return *m_materialBuffer;
}

void Locator::setWindow(Window* window)
{
	assert(m_window == nullptr);
	m_window = window;
}

void Locator::setRenderer(Renderer* renderer)
{
	assert(m_renderer == nullptr);
	m_renderer = renderer;
}

void Locator::setContext(Context* context)
{
	assert(m_context == nullptr);
	m_context = context;
}

void Locator::setDevice(Device* device)
{
	assert(m_device == nullptr);
	m_device = device;
}

void Locator::setSwapchain(Swapchain* swapchain)
{
	assert(m_swapchain == nullptr);
	m_swapchain = swapchain;
}

void Locator::setDescriptorPool(DescriptorPool* descriptorPool)
{
	assert(m_descriptorPool == nullptr);
	m_descriptorPool = descriptorPool;
}

void Locator::setTextureArray(TextureArray* textureArray)
{
	assert(m_textureArray == nullptr);
	m_textureArray = textureArray;
}

void Locator::setECS(entt::registry* ecs)
{
	assert(m_ecs == nullptr);
	m_ecs = ecs;
}

void Locator::setMaterialBuffer(DUB<Material>* materialBuffer)
{
	assert(m_materialBuffer == nullptr);
	m_materialBuffer = materialBuffer;
}