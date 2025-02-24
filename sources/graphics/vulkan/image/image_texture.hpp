#pragma once

#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/image/texture.hpp"

#include <string>

class ImageTexture : public Texture
{
public:
	~ImageTexture();
	void init(const std::string& imagePath, DescriptorSetPtr descriptorSet, uint32_t binding = 0);
	void destroy();

	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId) override;
	VkImageView getImageView() override;
	VkSampler getSampler() override;

private:
	void createImage(const std::string& imagePath);
	void createImageView(VkImageAspectFlags aspect);
	void createImageSampler(bool depth);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);
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