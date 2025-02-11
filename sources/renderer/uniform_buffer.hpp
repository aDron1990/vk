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
		auto& buffer = m_buffer;
		m_descriptorSet = m_device.createDescriptorSet(layout);
		buffer.reset(new Buffer{ m_device, m_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT });
		m_bufferMapped = buffer->map();

		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = buffer->getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(T);

		auto descriptorWrite = VkWriteDescriptorSet{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet.set;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.descriptorCount = 1;

		vkUpdateDescriptorSets(m_device.getDevice(), 1, &descriptorWrite, 0, nullptr);
	}

	~UniformBuffer()
	{
		m_descriptorSet.free();
	}

	void write(const T& data)
	{
		memcpy(m_bufferMapped, &data, sizeof(T));
	}

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set, 1, &m_descriptorSet.set, 0, nullptr);
	}

	Buffer& getBuffer()
	{
		return *m_buffer;
	}

	void* getBufferPtr()
	{
		return m_bufferMapped;
	}

	size_t getSize()
	{
		return m_size;
	}

private:
	Device& m_device;
	std::unique_ptr<Buffer> m_buffer;
	void* m_bufferMapped;
	DescriptorSet m_descriptorSet;
	VkDeviceSize m_size;
};