#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>
#include <algorithm>

Swapchain::Swapchain(Device& device, SwapchainProperties properties, std::function<void(uint32_t, uint32_t)> onResize)
	: m_device{device}, m_renderPass{ properties.renderPass }, m_onResize{onResize}
{
	createSwapchain();
	createImageViews();
	createDepthResources();
	createFramebuffers();
	Locator::setSwapchain(this);
}

Swapchain::~Swapchain()
{
	clear();
}

void Swapchain::clear()
{
	vkDestroyImageView(m_device.getDevice(), m_depthImageView, nullptr);
	vkDestroyImage(m_device.getDevice(), m_depthImage, nullptr);
	vkFreeMemory(m_device.getDevice(), m_depthImageMemory, nullptr);
	for (auto framebuffer : m_swapchainFramebuffers)
		vkDestroyFramebuffer(m_device.getDevice(), framebuffer, nullptr);
	for (auto view : m_swapchainImageViews)
		vkDestroyImageView(m_device.getDevice(), view, nullptr);
	vkDestroySwapchainKHR(m_device.getDevice(), m_swapchain, nullptr);
}

void Swapchain::recreate()
{
	vkDeviceWaitIdle(m_device.getDevice());
	clear();
	createSwapchain();
	createImageViews();
	createDepthResources();
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

	vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device.getDevice(), m_swapchain, &imageCount, m_swapchainImages.data());
	m_swapchainFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void Swapchain::createImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());
	for (int i = 0; i < m_swapchainImages.size(); i++)
	{
		m_swapchainImageViews[i] = m_device.createImageView(m_swapchainImages[i], m_swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void Swapchain::createFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchainImages.size());
	for (int i = 0; i < m_swapchainImages.size(); i++)
	{
		auto attachments = { m_swapchainImageViews[i], m_depthImageView };

		auto createInfo = VkFramebufferCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.pAttachments = attachments.begin();
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.width = m_swapchainExtent.width;
		createInfo.height = m_swapchainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_device.getDevice(), &createInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create framebuffer" };
	}
}

void Swapchain::createDepthResources()
{
	auto depthFormat = m_device.findDepthFormat();
	m_device.createImage(m_swapchainExtent.width, m_swapchainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
	m_depthImageView = m_device.createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	m_device.transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
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

VkFramebuffer Swapchain::getFramebuffer(uint32_t index)
{
	return m_swapchainFramebuffers[index];
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