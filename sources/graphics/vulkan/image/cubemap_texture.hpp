#pragma once

#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/image/texture.hpp"

#include <string>

class CubemapTexture : public Texture
{
public:
	~CubemapTexture();
	void init(const std::string& imageDirPath, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	void destroy();

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId) override;
	VkImageView getImageView() override;
	VkSampler getSampler() override;

private:
	void createImage(const std::string& imageDirPath);
	void createImageView(VkImageAspectFlags aspect);
	void createImageSampler();
	void writeDescriptorSet(uint32_t binding);

private:
	bool m_initialized = false;
	const uint32_t WIDTH = 1024;
	const uint32_t HEIGHT = 1024;
	Device* m_device;
	VkFormat m_format{};

	VkImage m_image{};
	VkDeviceMemory m_imageMemory{};
	VkImageView m_imageView{};
	VkSampler m_sampler{};
	uint32_t m_mipLevels{};
	DescriptorSetPtr m_descriptorSet{};
};