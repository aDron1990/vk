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
	Texture& texture;
};

class Pipeline
{
public:
	Pipeline(Device& device, PipelineInfo& info);
	~Pipeline();
	void bind(VkCommandBuffer commandBuffer, uint32_t currentFrame);
	void updateBuffer(uint32_t currentFrame);
	VkPipelineLayout getLayout();
	
private:
	void createPipeline();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	Device& m_device;
	PipelineInfo m_info;
	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets;

	UniformBuffer<UniformBufferObject> m_uniformBuffer;
	Texture& m_texture;
};