#pragma once 

#include "renderer/config.hpp"
#include "renderer/buffer.hpp"
#include "renderer/device.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>

template<typename T>
class UniformBuffer
{
public:
	UniformBuffer(Device& device, VkDescriptorSetLayout	layout) : m_device{device}
	{
		m_size = sizeof(T);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto& buffer = m_buffers[i];
			m_descriptorSets[i] = m_device.createDescriptorSet(layout);
			buffer.reset(new Buffer{ m_device, m_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT });
			m_buffersMapped[i] = buffer->map();

			auto bufferInfo = VkDescriptorBufferInfo{};
			bufferInfo.buffer = buffer->getBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(T);

			auto descriptorWrite = VkWriteDescriptorSet{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_descriptorSets[i].set;
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.descriptorCount = 1;

			vkUpdateDescriptorSets(m_device.getDevice(), 1, &descriptorWrite, 0, nullptr);
		}
	}

	~UniformBuffer()
	{
		for (auto set : m_descriptorSets)
			set.free();
	}

	void write(const T& data, uint32_t currentFrame)
	{
		memcpy(m_buffersMapped[currentFrame], &data, sizeof(T));
	}

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set, uint32_t currentFrame)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set, 1, &m_descriptorSets[currentFrame].set, 0, nullptr);
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
	std::array<DescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets;
	VkDeviceSize m_size;
};