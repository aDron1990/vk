#pragma once

#include "renderer/device.hpp"

class OffscreenPass
{
public:
	OffscreenPass(Device& device, uint32_t width, uint32_t height);
	~OffscreenPass();
	void begin(VkCommandBuffer commandBuffer, const std::array<VkClearValue, 2>& clearValues);
	void end(VkCommandBuffer commandBuffer);
	void bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set);
	VkRenderPass getRenderPass();

private:
	void createRenderPass();
	void createImage();
	void createDepthImage();
	void createFramebuffer();
	void createSampler();
	void createDescriptorSet();

private:
	Device& m_device;
	VkRenderPass m_renderPass;

	uint32_t m_width;
	uint32_t m_height;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_imageView;
	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;
	VkFramebuffer m_framebuffer;
	VkSampler m_sampler;
	DescriptorSet m_descriptorSet;
};