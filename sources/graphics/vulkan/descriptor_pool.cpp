#include "graphics/vulkan/descriptor_pool.hpp"
#include "graphics/vulkan/device.hpp"	

#include <stdexcept>

DescriptorPool::DescriptorPool(Device& device)
: m_device{device}
{
	createPool();
}

DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(m_device.getDevice(), m_pool, nullptr);
}

void DescriptorPool::createPool()
{
	auto poolSizes = std::vector<VkDescriptorPoolSize>(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	auto createInfo = VkDescriptorPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.maxSets = 4096;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	if (vkCreateDescriptorPool(m_device.getDevice(), &createInfo, nullptr, &m_pool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor pool" };
}

DescriptorSet DescriptorPool::createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
	auto descriptorSet = DescriptorSet{};
	auto allocInfo = VkDescriptorSetAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(m_device.getDevice(), &allocInfo, &descriptorSet.set) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate descriptor sets" };

	descriptorSet.free = [device = m_device.getDevice(), pool=m_pool, set = descriptorSet.set]()
	{
		vkFreeDescriptorSets(device, pool, 1, &set);
	};
	return descriptorSet;
}

VkDescriptorPool DescriptorPool::getPool()
{
	return m_pool;
}