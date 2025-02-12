#pragma once

#include "renderer/render_pass.hpp"
#include "renderer/device.hpp"

#include <vector>

class OffscreenPass : public RenderPass
{
public:
	OffscreenPass(Device& device, uint32_t width, uint32_t height, uint32_t colorImageCount = 1, bool storeDepth = false);
	~OffscreenPass();
	void begin(VkCommandBuffer commandBuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	void bindColorImage(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set, uint32_t index);
	void bindDepthImage(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set);
	void resize(uint32_t newWidth, uint32_t newHeight);
	VkRenderPass getRenderPass();

private:
	void createRenderPass();
	void clearResources();
	void createResources();
	void createImages();
	void createDepthImages();
	void createFramebuffers();
	void createSamplers();
	void createDescriptorSets();

private:
	Device& m_device;
	VkRenderPass m_renderPass;
	const bool m_storeDepth;

	uint32_t m_width;
	uint32_t m_height;

	struct AttachmentImage
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkSampler sampler;
		DescriptorSet descriptorSet;
	};

	std::vector<AttachmentImage> m_colorImages;
	AttachmentImage m_depthImage;
	
	//VkImage m_image;
	//VkDeviceMemory m_imageMemory;
	//VkImageView m_imageView;
	//VkImage m_depthImage;
	//VkDeviceMemory m_depthImageMemory;
	//VkImageView m_depthImageView;
	VkFramebuffer m_framebuffer;
	//VkSampler m_sampler;
	//DescriptorSet m_descriptorSet;
};