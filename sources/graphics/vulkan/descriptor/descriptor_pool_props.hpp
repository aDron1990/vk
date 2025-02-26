#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct BindingInfo
{
	VkDescriptorType descriptorType;
	uint32_t descriptorCount = 1;
};

struct DescriptorSetInfo
{
	std::vector<BindingInfo> bindings;
	VkPipelineStageFlags stages;
	uint32_t setCount;
};

struct DescriptorPoolProps
{
	std::vector<DescriptorSetInfo> setInfos;
};