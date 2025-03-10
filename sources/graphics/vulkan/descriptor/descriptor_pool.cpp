#include "graphics/vulkan/descriptor/descriptor_pool.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>
#include <vector>
#include <ranges>

DescriptorPool::~DescriptorPool()
{
	destroy();
}

void DescriptorPool::destroy()
{
	if (m_initialized)
	{
		vkDestroyDescriptorPool(m_device->getDevice(), m_pool, nullptr);
		for (auto layout : m_layouts)
			vkDestroyDescriptorSetLayout(m_device->getDevice(), layout, nullptr);
	}
	m_initialized = false;
}

void DescriptorPool::init(const DescriptorPoolProps& props)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_props = props;
	createPool();
	createDescriptorSetLayouts();
	
	Locator::setDescriptorPool(this);
}

void DescriptorPool::createPool()
{
	// Calculate each descriptor type count
	auto poolSizesMap = std::unordered_map<VkDescriptorType, uint32_t>{};
	for (auto& set : m_props.setInfos)
	{
		for (auto& binding : set.bindings)
		{
			if (poolSizesMap.count(binding.descriptorType) == 0) poolSizesMap[binding.descriptorType] = 0;
			poolSizesMap[binding.descriptorType] += set.setCount;
		}
	}

	auto poolSizes = std::vector<VkDescriptorPoolSize>(poolSizesMap.size());
	uint32_t totalSetCount = 0;
	for (auto [sizeMap, poolSize] : std::views::zip(poolSizesMap, poolSizes))
	{
		poolSize.type = sizeMap.first;
		poolSize.descriptorCount = sizeMap.second;
		totalSetCount += poolSize.descriptorCount;
	}

	auto createInfo = VkDescriptorPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.maxSets = totalSetCount;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	if (vkCreateDescriptorPool(m_device->getDevice(), &createInfo, nullptr, &m_pool) != VK_SUCCESS)
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
		if (vkCreateDescriptorSetLayout(m_device->getDevice(), &createInfo, nullptr, &m_layouts[setId]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create descriptor set layout" };
	}
}

DescriptorSetPtr DescriptorPool::createSet(uint32_t setId)
{
	auto set = VkDescriptorSet{};
	auto allocInfo = VkDescriptorSetAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pool;
	allocInfo.pSetLayouts = &m_layouts[setId];
	allocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, &set) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate descriptor sets" };

	auto free = [device = m_device->getDevice(), pool = m_pool, set = set]()
	{
		vkFreeDescriptorSets(device, pool, 1, &set);
	};

	auto descriptorSet = std::make_shared<DescriptorSet>(free, set);
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