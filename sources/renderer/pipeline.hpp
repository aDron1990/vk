#pragma once

#include "renderer/config.hpp"
#include "renderer/device.hpp"
#include "renderer/uniform_buffer.hpp"
#include "renderer/texture.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct PipelineInfo
{
	std::string vertexPath;
	std::string fragmentPath;
	VkRenderPass renderPass;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	bool vertexInput;
	VkCullModeFlags culling;
	uint32_t attachmentCount;
};

class Pipeline
{
public:
	Pipeline(Device& device, PipelineInfo& info);
	~Pipeline();
	void bind(VkCommandBuffer commandBuffer);
	VkPipelineLayout getLayout();
	
protected:
	void createPipeline();

private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	Device& m_device;
	PipelineInfo m_info;
	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};