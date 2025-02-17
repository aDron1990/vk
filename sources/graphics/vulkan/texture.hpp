#pragma once

#include "graphics/vulkan/context/device.hpp"

#include <string>

enum AttachmentType
{
	Color,
	Depth
};

class Texture
{
public:
	~Texture();
	void init(const std::string& imagePath, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	void init(AttachmentType attachmentType, uint32_t width, uint32_t height, VkFormat format, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	void init(VkImage swapchainImage, VkFormat format);
	void destroy();	

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId);
	VkImageView getImageView();
	VkSampler getSampler();

private:
	void createImage(const std::string& imagePath);
	void createImage(AttachmentType attachmentType, uint32_t width, uint32_t height);
	void createImageView(VkImageAspectFlags aspect);
	void createImageSampler(bool depth);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);
	void writeDescriptorSet(uint32_t binding);

private:
	Device* m_device;
	VkFormat m_format{};
	bool m_initialized = false;
	bool m_isSwapchainImage = false;

	VkImage m_image{};
	VkDeviceMemory m_imageMemory{};
	VkImageView m_imageView{};
	VkSampler m_sampler{};
	uint32_t m_mipLevels{};
	DescriptorSetPtr m_descriptorSet{};
};