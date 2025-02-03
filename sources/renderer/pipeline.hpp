#pragma once

#include "renderer/config.hpp"
#include "renderer/device.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct PipelineInfo
{
	std::string vertexPath;
	std::string fragmentPath;
};

class Pipeline
{
public:
	Pipeline(Device& device, PipelineInfo& info);

private:
	Device& m_device;
	PipelineInfo m_info;
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::vector<VkDescriptorSet> m_decsriptorSets;
};