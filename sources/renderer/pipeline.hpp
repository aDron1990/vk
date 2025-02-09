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
};

class Pipeline
{
public:
	Pipeline(Device& device, PipelineInfo& info);
	virtual ~Pipeline();
	void bind(VkCommandBuffer commandBuffer, uint32_t currentFrame);
	VkPipelineLayout getLayout();
	
protected:
	void createPipeline(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	Device& m_device;
	PipelineInfo m_info;
	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};