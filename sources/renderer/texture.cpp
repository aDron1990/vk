#include "renderer/texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

Texture::Texture(Device& device, const std::string& imagePath) : m_device{device}
{
	createImage(imagePath);
	createImageView();
	createImageSampler();
}

Texture::~Texture()
{
	vkDestroySampler(m_device.getDevice(), m_sampler, nullptr);
	vkDestroyImageView(m_device.getDevice(), m_imageView, nullptr);
	vkDestroyImage(m_device.getDevice(), m_image, nullptr);
	vkFreeMemory(m_device.getDevice(), m_imageMemory, nullptr);
}

void Texture::createImage(const std::string& imagePath)
{
	int width, height, channels;
	auto* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	VkDeviceSize size = width * height * 4;
	if (pixels == nullptr)
		throw std::runtime_error{ "failed to load image" };

	auto stagingBuffer = Buffer{ m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	auto* data = stagingBuffer.map();
	memcpy(data, pixels, static_cast<size_t>(size));
	stagingBuffer.unmap();
	stbi_image_free(pixels);

	m_device.createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory
	);

	m_device.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_device.copyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	m_device.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Texture::createImageView()
{
	m_imageView = m_device.createImageView(m_image, VK_FORMAT_R8G8B8A8_SRGB);
}

void Texture::createImageSampler()
{
	auto gpuProps = VkPhysicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(m_device.getGpu(), &gpuProps);

	auto createInfo = VkSamplerCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = gpuProps.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;
	if (vkCreateSampler(m_device.getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

VkImageView Texture::getImageView()
{
	return m_imageView;
}

VkSampler Texture::getSampler()
{
	return m_sampler;
}