#pragma once

#include "renderer/pipeline.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class GraphicsPipeline : public Pipeline
{
public:
	GraphicsPipeline(Device& device, PipelineInfo& info);
	~GraphicsPipeline() override;
	void bindTexture(Texture& texture);
	void updateBuffer(uint32_t currentFrame);

private:
	void createDescriptorSetLayouts();
	void createDescriptorPool();
	void createDescriptorSets();
	void bindDescriptorSets(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;

private:
	Device& m_device;
	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_uboDescriptorSetLayout;
	VkDescriptorSetLayout m_textureDescriptorSetLayout;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_uboDescriptorSets;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_textureDescriptorSets;

	UniformBuffer<UniformBufferObject> m_uniformBuffer;
};