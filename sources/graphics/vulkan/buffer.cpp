#include "graphics/vulkan/buffer.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>

Buffer::~Buffer()
{
	destroy();
}

void Buffer::destroy()
{
	if (m_initialized)
	{
		vkDestroyBuffer(m_device->getDevice(), m_buffer, nullptr);
		vkFreeMemory(m_device->getDevice(), m_bufferMemory, nullptr);
	}
	m_initialized = false;
}

void Buffer::init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	createBuffer(size, usage, properties);
}

void Buffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	auto createInfo = VkBufferCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_device->getDevice(), &createInfo, nullptr, &m_buffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vertex buffer" };

	auto memReq = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(m_device->getDevice(), m_buffer, &memReq);
	m_size = memReq.size;

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = m_size;
	allocInfo.memoryTypeIndex = m_device->findMemoryType(memReq.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device->getDevice(), &allocInfo, nullptr, &m_bufferMemory) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vertex buffer memory" };

	vkBindBufferMemory(m_device->getDevice(), m_buffer, m_bufferMemory, 0);
}

void* Buffer::map()
{
	assert(m_initialized);
	void* data{};
	if (vkMapMemory(m_device->getDevice(), m_bufferMemory, 0, m_size, 0, &data) != VK_SUCCESS)
		throw std::runtime_error{ "failed to map buffer memory" };

	return data;
}

void Buffer::unmap()
{
	assert(m_initialized);
	vkUnmapMemory(m_device->getDevice(), m_bufferMemory);
}

VkBuffer Buffer::getBuffer()
{
	assert(m_initialized);
	return m_buffer;
}

VkDeviceSize Buffer::getSize()
{
	assert(m_initialized);
	return m_size;
}