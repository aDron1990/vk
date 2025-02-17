#pragma once 

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/buffer.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/locator.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>

template<typename T>
class UniformBuffer
{
public:
	void init(DescriptorSetPtr descriptorSet, uint32_t binding = 0)
	{
		assert(!m_initialized);
		m_initialized = true;
		m_device = &Locator::getDevice();
		m_descriptorSet = descriptorSet;

		m_size = sizeof(T);
		m_buffer.init(m_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_bufferMapped = m_buffer.map();

		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = m_buffer.getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(T);

		auto descriptorWrite = VkWriteDescriptorSet{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet->getSet();
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.descriptorCount = 1;

		vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
	}

	void write(const T& data)
	{
		memcpy(m_bufferMapped, &data, sizeof(T));
	}

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
	{
		auto set = m_descriptorSet->getSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 0, nullptr);
	}

	Buffer& getBuffer()
	{
		return m_buffer;
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
	bool m_initialized = false;
	Device* m_device{};
	Buffer m_buffer{};
	void* m_bufferMapped{};
	DescriptorSetPtr m_descriptorSet{};
	VkDeviceSize m_size{};
};