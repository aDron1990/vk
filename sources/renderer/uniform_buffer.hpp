#pragma once 

#include "renderer/config.hpp"
#include "renderer/buffer.hpp"
#include "renderer/device.hpp"

#include <array>
#include <memory>

template<typename T>
class UniformBuffer
{
public:
	UniformBuffer(Device& device) : m_device{device}
	{
		m_size = sizeof(T);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto& buffer = m_buffers[i];
			buffer.reset(new Buffer{ m_device, m_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT });
			m_buffersMapped[i] = buffer->map();
		}
	}

	Buffer& getBuffer(uint32_t currentFrame)
	{
		return *m_buffers[currentFrame];
	}

	void* getBufferPtr(uint32_t currentFrame)
	{
		return m_buffersMapped[static_cast<size_t>(currentFrame)];
	}

	size_t getSize()
	{
		return m_size;
	}

private:
	Device& m_device;
	std::array<std::unique_ptr<Buffer>, MAX_FRAMES_IN_FLIGHT> m_buffers;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_buffersMapped;
	VkDeviceSize m_size;
};