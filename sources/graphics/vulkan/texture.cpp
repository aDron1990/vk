#include "graphics/vulkan/texture.hpp"
#include "graphics/vulkan/locator.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(images);

#include <stdexcept>
#include <cassert>

Texture::~Texture()
{
    destroy();
}

void Texture::destroy()
{
    if (m_initialized)
    {
        vkDestroyImageView(m_device->getDevice(), m_imageView, nullptr);
        if (!m_isSwapchainImage)
        {
            vkDestroySampler(m_device->getDevice(), m_sampler, nullptr);
            vkDestroyImage(m_device->getDevice(), m_image, nullptr);
            vkFreeMemory(m_device->getDevice(), m_imageMemory, nullptr);
        }
    }
    m_initialized = false;
    m_isSwapchainImage = false;
}

void Texture::init(const std::string& imagePath, DescriptorSetPtr descriptorSet, uint32_t binding)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_descriptorSet = descriptorSet;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;
    createImage(imagePath);
    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    createImageSampler();
    writeDescriptorSet(binding);
}

void Texture::init(AttachmentType attachmentType, uint32_t width, uint32_t height, VkFormat format, DescriptorSetPtr descriptorSet, uint32_t binding)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_descriptorSet = descriptorSet;
    m_format = format;
    m_mipLevels = 1;
    createImage(attachmentType, width, height);
    createImageView(attachmentType == AttachmentType::Color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT);
    createImageSampler();
    writeDescriptorSet(binding);
    m_initialized = true;
}

void Texture::init(VkImage swapchainImage, VkFormat format)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_isSwapchainImage = true;
    m_image = swapchainImage;
    m_format = format;
    m_mipLevels = 1;
    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    m_initialized = true;
}

void Texture::createImage(const std::string& imagePath)
{
    int width, height, channels;
    auto imageFile = cmrc::images::get_filesystem().open(imagePath);
    auto* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(imageFile.begin()), imageFile.size(), &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize size = width * height * 4;
    if (pixels == nullptr)
        throw std::runtime_error{ "failed to load image" };

    auto stagingBuffer = Buffer{};
    stagingBuffer.init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto* data = stagingBuffer.map();
    memcpy(data, pixels, static_cast<size_t>(size));
    stagingBuffer.unmap();
    stbi_image_free(pixels);

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    m_device->createImage(width, height, m_mipLevels, m_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory
    );

    m_device->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
    m_device->copyBufferToImage(stagingBuffer, m_image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    generateMipmaps(m_image, m_format, width, height, m_mipLevels);
}

void Texture::createImage(AttachmentType attachmentType, uint32_t width, uint32_t height)
{
    auto usage = VkImageUsageFlags{};
    usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (attachmentType == AttachmentType::Color)
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if (attachmentType == AttachmentType::Depth)
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else assert(false && "wrong attachment type");

    m_device->createImage(
        width, height, m_mipLevels, m_format, VK_IMAGE_TILING_OPTIMAL, 
        usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory
    );

    auto aspect = attachmentType == AttachmentType::Color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    m_device->transitionImageLayout(m_image, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels, aspect);
}

void Texture::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels)
{
    auto formatProperties = VkFormatProperties{};
    vkGetPhysicalDeviceFormatProperties(m_device->getGpu(), imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        throw std::runtime_error("texture image format does not support linear blitting!");

    auto commandBuffer = m_device->beginSingleTimeCommands();

    auto barrier = VkImageMemoryBarrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;
    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        auto blit = VkImageBlit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    m_device->endSingleTimeCommands(commandBuffer);
}

void Texture::createImageView(VkImageAspectFlags aspect)
{
	m_imageView = m_device->createImageView(m_image, m_format, aspect, m_mipLevels);
}

void Texture::createImageSampler()
{
	auto gpuProps = VkPhysicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(m_device->getGpu(), &gpuProps);

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
	createInfo.maxLod = static_cast<float>(m_mipLevels);
	if (vkCreateSampler(m_device->getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

void Texture::writeDescriptorSet(uint32_t binding)
{
    auto samplerInfo = VkDescriptorImageInfo{};
    samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerInfo.imageView = m_imageView;
    samplerInfo.sampler = m_sampler;

    auto descriptorWrite = VkWriteDescriptorSet{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet->getSet();
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &samplerInfo;
    descriptorWrite.descriptorCount = 1;
    vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void Texture::bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
{
    assert(m_initialized);
    auto set = m_descriptorSet->getSet();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 0, nullptr);
}

VkImageView Texture::getImageView()
{
    assert(m_initialized);
	return m_imageView;
}

VkSampler Texture::getSampler()
{
    assert(m_initialized);
    return m_sampler;
}