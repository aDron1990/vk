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
	VkDescriptorSetLayout descriptorSetLayout;
	VkRenderPass renderPass;
};

class Pipeline
{
public:
	Pipeline(Device& device, PipelineInfo& info);
	~Pipeline();
	void bind(VkCommandBuffer commandBuffer);
	VkPipelineLayout getLayout();

private:
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	Device& m_device;
	PipelineInfo m_info;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
};