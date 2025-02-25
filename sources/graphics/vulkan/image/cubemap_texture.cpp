#include "graphics/vulkan/image/cubemap_texture.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stb_image.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(images);

#include <stdexcept>
#include <ranges>
#include <cassert>

CubemapTexture::~CubemapTexture()
{
    destroy();
}

void CubemapTexture::destroy()
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

void CubemapTexture::init(const std::string& imageDirPath, DescriptorSetPtr descriptorSet, uint32_t binding)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_descriptorSet = descriptorSet;
    m_format = VK_FORMAT_R8G8B8A8_SRGB;
    createImage(imageDirPath);
    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
    createImageSampler();
    writeDescriptorSet(binding);
}

void CubemapTexture::createImage(const std::string& imageDirPath)
{
    using namespace std::literals;
    auto sides = { "right", "left", "top", "bottom", "forward", "back" };
    VkDeviceSize sideSize = WIDTH * WIDTH * 4;
    VkDeviceSize size = sideSize * 6;
    auto buffer = std::vector<stbi_uc>(size);
    for (auto [i, side] : std::views::enumerate(sides))
    {
        auto imageFile = cmrc::images::get_filesystem().open(imageDirPath + "/"s + side + ".png"s);
        int width, height, channels;
        auto* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(imageFile.begin()), imageFile.size(), &width, &height, &channels, STBI_rgb_alpha);
        assert(WIDTH == width);
        assert(HEIGHT == height);
        if (pixels == nullptr)
            throw std::runtime_error{ "failed to load image" };
        std::copy(pixels, pixels + sideSize, buffer.begin() + (i * sideSize));
        stbi_image_free(pixels);
    }

    auto stagingBuffer = Buffer{};
    stagingBuffer.init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    auto* data = stagingBuffer.map();
    memcpy(data, buffer.data(), buffer.size());
    stagingBuffer.unmap();

    m_mipLevels = 1;
    m_device->createImage(WIDTH, HEIGHT, m_mipLevels, 6, m_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory
    );
    m_device->transitionImageLayout(m_image, 6, m_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);
    m_device->copyBufferToImage(stagingBuffer, m_image, WIDTH, HEIGHT, 6);
    m_device->transitionImageLayout(m_image, 6, m_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels);
}

void CubemapTexture::createImageView(VkImageAspectFlags aspect)
{
    m_imageView = m_device->createImageView(m_image, 6, m_format, aspect, m_mipLevels);
}

void CubemapTexture::createImageSampler()
{
    auto gpuProps = VkPhysicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(m_device->getGpu(), &gpuProps);

    auto createInfo = VkSamplerCreateInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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

void CubemapTexture::writeDescriptorSet(uint32_t binding)
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

void CubemapTexture::bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
{
    assert(m_initialized);
    auto set = m_descriptorSet->getSet();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 0, nullptr);
}

VkImageView CubemapTexture::getImageView()
{
    assert(m_initialized);
    return m_imageView;
}

VkSampler CubemapTexture::getSampler()
{
    assert(m_initialized);
    return m_sampler;
}