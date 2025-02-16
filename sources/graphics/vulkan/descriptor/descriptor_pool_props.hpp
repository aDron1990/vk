#pragma once

#include <vulkan/vulkan.h>

#include <vector>

struct BindingInfo
{
	VkDescriptorType descriptorType;
};

struct DescriptorSetInfo
{
	std::vector<BindingInfo> bindings;
	VkPipelineStageFlags stages;
};

struct DescriptorPoolProps
{
	std::vector<DescriptorSetInfo> setInfos;
	uint32_t setCountMultiplier;
};