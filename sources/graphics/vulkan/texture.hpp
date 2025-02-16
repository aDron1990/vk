#pragma once

#include "graphics/vulkan/device.hpp"

#include <string>

class Texture
{
public:
	Texture(Device& device, const std::string& imagePath, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	~Texture();
	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId);
	VkImageView getImageView();
	VkSampler getSampler();

private:
	void createImage(const std::string& imagePath);
	void createImageView();
	void createImageSampler();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);
	void writeDescriptorSet(uint32_t binding);

private:
	Device& m_device;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_imageView;
	VkSampler m_sampler;
	uint32_t m_mipLevels;
	DescriptorSetPtr m_descriptorSet;
};