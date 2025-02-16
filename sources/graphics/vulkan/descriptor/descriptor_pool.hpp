#pragma once

#include "graphics/vulkan/descriptor/descriptor_pool_props.hpp"

#include <vulkan/vulkan.h>

#include <functional>

struct DescriptorSet
{
	VkDescriptorSet set;
	std::function<void()> free;
};

class Device;

class DescriptorPool
{
public:
	DescriptorPool(Device& device, const DescriptorPoolProps& props);
	~DescriptorPool();

	DescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);
	VkDescriptorPool getPool();

private:
	void createPool();
	void createDescriptorSetLayouts();

private:
	Device& m_device;
	VkDescriptorPool m_pool;
	DescriptorPoolProps m_props;
	std::vector<VkDescriptorSetLayout> m_setLayouts;
};