#include "graphics/vulkan/image/render_texture.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>
#include <cassert>

RenderTexture::~RenderTexture()
{
    destroy();
}

void RenderTexture::destroy()
{
    if (m_initialized)
    {
        vkDestroyImageView(m_device->getDevice(), m_imageView, nullptr);
        vkDestroySampler(m_device->getDevice(), m_sampler, nullptr);
        vkDestroyImage(m_device->getDevice(), m_image, nullptr);
        vkFreeMemory(m_device->getDevice(), m_imageMemory, nullptr);
    }
    m_initialized = false;
}

void RenderTexture::init(AttachmentType attachmentType, uint32_t width, uint32_t height, VkFormat format, DescriptorSetPtr descriptorSet, uint32_t binding)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_descriptorSet = descriptorSet;
    m_format = format;
    m_mipLevels = 1;
    createImage(attachmentType, width, height);
    createImageView(attachmentType == AttachmentType::Color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT);
    createImageSampler(attachmentType == AttachmentType::Depth);
    writeDescriptorSet(binding);
    m_initialized = true;
}

void RenderTexture::init(VkImage swapchainImage, VkFormat format)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_image = swapchainImage;
    m_format = format;
    m_mipLevels = 1;
    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    m_initialized = true;
}

void RenderTexture::createImage(AttachmentType attachmentType, uint32_t width, uint32_t height)
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

void RenderTexture::createImageView(VkImageAspectFlags aspect)
{
    m_imageView = m_device->createImageView(m_image, m_format, aspect, m_mipLevels);
}

void RenderTexture::createImageSampler(bool depth)
{
    auto gpuProps = VkPhysicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(m_device->getGpu(), &gpuProps);

    auto createInfo = VkSamplerCreateInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = depth ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = depth ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = depth ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = gpuProps.limits.maxSamplerAnisotropy;
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

void RenderTexture::writeDescriptorSet(uint32_t binding)
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

void RenderTexture::bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
{
    assert(m_initialized);
    auto set = m_descriptorSet->getSet();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 0, nullptr);
}

VkImageView RenderTexture::getImageView()
{
    assert(m_initialized);
    return m_imageView;
}

VkSampler RenderTexture::getSampler()
{
    assert(m_initialized);
    return m_sampler;
}