#include "graphics/vulkan/image/swapchain_image.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>
#include <cassert>

SwapchainImage::~SwapchainImage()
{
    destroy();
}

void SwapchainImage::destroy()
{
    if (m_initialized)
    {
        vkDestroyImageView(m_device->getDevice(), m_imageView, nullptr);
    }
    m_initialized = false;
}

void SwapchainImage::init(VkImage swapchainImage, VkFormat format)
{
    assert(!m_initialized);
    m_initialized = true;
    m_device = &Locator::getDevice();
    m_image = swapchainImage;
    m_format = format;
    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void SwapchainImage::createImageView(VkImageAspectFlags aspect)
{
    m_imageView = m_device->createImageView(m_image, m_format, aspect, 1);
}

VkImageView SwapchainImage::getImageView()
{
    assert(m_initialized);
    return m_imageView;
}
