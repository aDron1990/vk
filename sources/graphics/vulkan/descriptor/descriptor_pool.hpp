#pragma once

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
	DescriptorPool(Device& device);
	~DescriptorPool();
	DescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);
	VkDescriptorPool getPool();

private:
	void createPool();

private:
	Device& m_device;
	VkDescriptorPool m_pool;

};