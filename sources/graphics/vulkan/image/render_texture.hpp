#pragma once

#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/image/texture.hpp"

#include <string>

enum class AttachmentType
{
	Color,
	Depth
};

class RenderTexture : public Texture
{
public:
	~RenderTexture();
	void init(AttachmentType attachmentType, uint32_t width, uint32_t height, VkFormat format, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	void init(VkImage swapchainImage, VkFormat format);
	void destroy();

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId) override;
	VkImageView getImageView() override;
	VkSampler getSampler() override;

private:
	void createImage(AttachmentType attachmentType, uint32_t width, uint32_t height);
	void createImageView(VkImageAspectFlags aspect);
	void createImageSampler(bool depth);
	void writeDescriptorSet(uint32_t binding);

private:
	bool m_initialized = false;
	Device* m_device;
	VkFormat m_format{};

	VkImage m_image{};
	VkDeviceMemory m_imageMemory{};
	VkImageView m_imageView{};
	VkSampler m_sampler{};
	uint32_t m_mipLevels{};
	DescriptorSetPtr m_descriptorSet{};
};