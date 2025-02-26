#pragma once 

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/buffer.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/locator.hpp"
#include "graphics/vulkan/utils.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>

template<typename T>
class DUB
{
public:
	void init(size_t count, DescriptorSetPtr descriptorSet, uint32_t binding = 0)
	{
		assert(!m_initialized);
		m_initialized = true;
		m_device = &Locator::getDevice();
		m_descriptorSet = descriptorSet;
		m_count = count;

		m_buffer.init(m_alignedSize * count, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_bufferMapped = m_buffer.map();

		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = m_buffer.getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = m_size;

		auto descriptorWrite = VkWriteDescriptorSet{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet->getSet();
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.descriptorCount = 1;

		vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
	}

	void write(size_t id, const T& data)
	{
		assert(m_initialized);
		assert(id < m_count);
		auto* bufferPtr = static_cast<char*>(m_bufferMapped);
		auto* writePtr = bufferPtr + id * m_alignedSize;
		memcpy(writePtr, &data, sizeof(T));
	}

	void bind(size_t id, VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
	{
		assert(m_initialized);
		assert(id < m_count);
		auto set = m_descriptorSet->getSet();
		uint32_t offset = id * m_alignedSize;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 1, &offset);
	}

	uint32_t genIndex()
	{
		assert(m_initialized);
		assert(m_nextIndex < m_count);
		return m_nextIndex++;
	}

	Buffer& getBuffer()
	{
		assert(m_initialized);
		return m_buffer;
	}

	void* getBufferPtr()
	{
		assert(m_initialized);
		return m_bufferMapped;
	}

	size_t getSize()
	{
		assert(m_initialized);
		return m_size;
	}

private:
	bool m_initialized = false;
	Device* m_device{};
	Buffer m_buffer{};
	void* m_bufferMapped{};
	DescriptorSetPtr m_descriptorSet{};
	const size_t m_size = sizeof(T);
	const size_t m_alignedSize = alignedSize<T>(64);
	size_t m_count{};
	uint32_t m_nextIndex{};
};