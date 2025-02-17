#pragma once

#include "graphics/vulkan/descriptor/descriptor_pool_props.hpp"

#include <vulkan/vulkan.h>

#include <functional>
#include <memory>

struct DescriptorSet
{
public:
	DescriptorSet(std::function<void()> free, VkDescriptorSet set) : m_free{ free }, m_set{set} {};
	VkDescriptorSet getSet() { return m_set; };

private:
	std::function<void()> m_free;
	VkDescriptorSet m_set;
};

using DescriptorSetPtr = std::shared_ptr<DescriptorSet>;

class Device;

class DescriptorPool
{
public:
	~DescriptorPool();
	void init(const DescriptorPoolProps& props);
	void destroy();

	DescriptorSetPtr createSet(uint32_t setId);
	VkDescriptorPool getPool();
	VkDescriptorSetLayout getLayout(uint32_t setId);
	std::vector<VkDescriptorSetLayout> getLayouts();

private:
	void createPool();
	void createDescriptorSetLayouts();

private:
	bool m_initialized = false;
	Device* m_device;
	VkDescriptorPool m_pool{};
	DescriptorPoolProps m_props{};
	std::vector<VkDescriptorSetLayout> m_layouts{};
};