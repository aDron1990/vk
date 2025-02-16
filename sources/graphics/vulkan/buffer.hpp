#pragma once

#include <vulkan/vulkan.h>

class Device;

class Buffer
{
public:
	Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	~Buffer();
	void* map();
	void unmap();
	VkBuffer getBuffer();
	VkDeviceSize getSize();

private:
	Device& m_device;
	VkBuffer m_buffer;
	VkDeviceMemory m_bufferMemory;
	VkDeviceSize m_size;
};