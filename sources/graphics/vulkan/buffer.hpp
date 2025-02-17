#pragma once

#include <vulkan/vulkan.h>

class Device;

class Buffer
{
public:
	~Buffer();
	void init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void destroy();

	void* map();
	void unmap();
	VkBuffer getBuffer();
	VkDeviceSize getSize();
	
private:
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

private:
	bool m_initialized = false;
	Device* m_device{};
	VkBuffer m_buffer{};
	VkDeviceMemory m_bufferMemory{};
	VkDeviceSize m_size{};
};