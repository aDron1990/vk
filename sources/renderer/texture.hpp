#pragma once

#include "renderer/device.hpp"

#include <string>

class Texture
{
public:
	Texture(Device& device, const std::string& imagePath);
	~Texture();
	VkImageView getImageView();
	VkSampler getSampler();

private:
	void createImage(const std::string& imagePath);
	void createImageView();
	void createImageSampler();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);

private:
	Device& m_device;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_imageView;
	VkSampler m_sampler;
	uint32_t m_mipLevels;
};