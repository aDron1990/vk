#include "graphics/vulkan/descriptor/descriptor_pool.hpp"
#include "graphics/vulkan/device.hpp"	

#include <stdexcept>
#include <vector>
#include <ranges>

DescriptorPool::DescriptorPool(Device& device, const DescriptorPoolProps& props)
: m_device{device}, m_props{props}
{
	createPool();
	createDescriptorSetLayouts();
}

DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(m_device.getDevice(), m_pool, nullptr);
	for (auto layout : m_layouts)
		vkDestroyDescriptorSetLayout(m_device.getDevice(), layout, nullptr);
}

void DescriptorPool::createPool()
{
	// Calculate each descriptor type count
	auto bindingCount = 0;
	auto poolSizesMap = std::unordered_map<VkDescriptorType, uint32_t>{};
	for (auto& set : m_props.setInfos)
	{
		for (auto& binding : set.bindings)
		{
			bindingCount++;
			if (poolSizesMap.count(binding.descriptorType) == 0) poolSizesMap[binding.descriptorType] = 1;
			else poolSizesMap[binding.descriptorType]++;
		}
	}

	auto poolSizes = std::vector<VkDescriptorPoolSize>(poolSizesMap.size());
	for (auto size : std::views::zip(poolSizesMap, poolSizes))
	{
		auto& poolSize = std::get<1>(size);
		poolSize.type = std::get<0>(size).first;
		poolSize.descriptorCount = std::get<0>(size).second * m_props.setCountMultiplier;
	}

	auto createInfo = VkDescriptorPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.maxSets = static_cast<uint32_t>(m_props.setCountMultiplier * bindingCount);
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	if (vkCreateDescriptorPool(m_device.getDevice(), &createInfo, nullptr, &m_pool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor pool" };
}

void DescriptorPool::createDescriptorSetLayouts()
{
	const auto setCount = m_props.setInfos.size();
	m_layouts.resize(setCount);
	for (size_t setId = 0; setId < setCount; setId++)
	{
		auto& setInfo = m_props.setInfos[setId];
		auto bindings = std::vector<VkDescriptorSetLayoutBinding>{};
		bindings.resize(setInfo.bindings.size());
		for (size_t i = 0; i < bindings.size(); i++)
		{
			auto bind = VkDescriptorSetLayoutBinding{};
			bind.binding = i;
			bind.descriptorType = setInfo.bindings[i].descriptorType;
			bind.descriptorCount = 1;
			bind.stageFlags = setInfo.stages;
			bindings[i] = bind;
		}

		auto createInfo = VkDescriptorSetLayoutCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.pBindings = bindings.data();
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		if (vkCreateDescriptorSetLayout(m_device.getDevice(), &createInfo, nullptr, &m_layouts[setId]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create descriptor set layout" };
	}
}

DescriptorSet DescriptorPool::createSet(VkDescriptorSetLayout descriptorSetLayout)
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

VkDescriptorSetLayout DescriptorPool::getLayout(uint32_t setId)
{
	return m_layouts[setId];
}

std::vector<VkDescriptorSetLayout> DescriptorPool::getLayouts()
{
	return m_layouts;
}

VkDescriptorPool DescriptorPool::getPool()
{
	return m_pool;
}