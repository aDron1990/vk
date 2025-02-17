#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>
#include <algorithm>
#include <ranges>

Swapchain::Swapchain(const FramebufferProps& framebufferProps, RenderPass& renderPass, std::function<void(uint32_t, uint32_t)> onResize)
	: m_device{ Locator::getDevice() }, m_framebufferProps{ framebufferProps },  m_renderPass { &renderPass }, m_onResize{ onResize }
{
	createSwapchain();
	createFramebuffers();
	Locator::setSwapchain(this);
}

Swapchain::~Swapchain()
{
	clear();
}

void Swapchain::clear()
{
	
	vkDestroySwapchainKHR(m_device.getDevice(), m_swapchain, nullptr);
}

void Swapchain::recreate()
{
	vkDeviceWaitIdle(m_device.getDevice());
	clear();
	createSwapchain();
	createFramebuffers();
}

void Swapchain::createSwapchain()
{
	auto swapchainDetails = m_device.querySwapchainSupport(m_device.getGpu());
	auto surfaceFormat = chooseSwapchainSurfaceFormat(swapchainDetails.formats);
	auto presentMode = chooseSwapchainPresentMode(swapchainDetails.presentModes);
	auto extent = chooseSwapchainExtent(swapchainDetails.capabilities);

	auto imageCount = swapchainDetails.capabilities.minImageCount + 1;
	if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount)
		imageCount = swapchainDetails.capabilities.maxImageCount;

	auto createInfo = VkSwapchainCreateInfoKHR{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_device.getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = m_device.findQueueFamilies(m_device.getGpu());
	if (indices.graphics != indices.present)
	{
		auto queueFamilyIndices = { indices.graphics.value(), indices.present.value() };
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = queueFamilyIndices.begin();
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	}
	else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device.getDevice(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan swapchain" };

	m_swapchainFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void Swapchain::createFramebuffers()
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapchain, &imageCount, nullptr);
	auto swapchainImages = std::vector<VkImage>(imageCount);
	vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapchain, &imageCount, swapchainImages.data());
	m_framebuffers.resize(imageCount);
	for (auto [image, framebuffer] : std::views::zip(swapchainImages, m_framebuffers))
	{
		framebuffer.init(m_framebufferProps, image, *m_renderPass, m_swapchainExtent.width, m_swapchainExtent.height);
	}
}

VkSurfaceFormatKHR Swapchain::chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	for (const auto& mode : availableModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		throw; // TODO getting framebuffer size from window
		//auto actualExtent = m_device.querySwapchainSupport(m_device.getGpu()).capabilities.currentExtent;
		//actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		//actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		//return actualExtent;
	}
}

VkExtent2D Swapchain::getExtent()
{
	return m_swapchainExtent;
}

VkFormat Swapchain::getFormat()
{
	return m_swapchainFormat;
}

Framebuffer& Swapchain::getFramebuffer(uint32_t index)
{
	return m_framebuffers[index];
}

uint32_t Swapchain::getImageIndex()
{
	return m_imageIndex;
}

uint32_t Swapchain::beginFrame(VkFence inFlightFence, VkSemaphore imageAvailableSemaphore)
{
	vkWaitForFences(m_device.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

	auto result = vkAcquireNextImageKHR(m_device.getDevice(), m_swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		recreate();
		m_onResize(m_swapchainExtent.width, m_swapchainExtent.height);
		return UINT32_MAX;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("failed to acquire swap chain image!");

	vkResetFences(m_device.getDevice(), 1, &inFlightFence);
	return m_imageIndex;
}

void Swapchain::endFrame(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore)
{
	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &imageIndex;
	vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);
}