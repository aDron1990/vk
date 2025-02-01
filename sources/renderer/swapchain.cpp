#include "renderer/swapchain.hpp"

#include <stdexcept>
#include <algorithm>

Swapchain::Swapchain(SwapchainProperties properties)
	: m_instance{ properties.instance }, m_gpu{ properties.gpu }, m_device{ properties.device }, m_commandPool{ properties.commandPool }, 
	m_renderPass{ properties.renderPass }, m_surface {properties.surface }, m_extent{ properties.extent }, 
	m_queueFamilyIndices{properties.queueFamilyIndices}, m_swapchainSupportDetails{properties.swapchainSupportDetails}
{
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.present.value(), 0, &m_presentQueue);
	createSwapchain();
	createImageViews();
	createFramebuffers();
	createSyncObjects();
	createCommandBuffers();
}

Swapchain::~Swapchain()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
	}
	for (auto framebuffer : m_swapchainFramebuffers)
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	for (auto view : m_swapchainImageViews)
		vkDestroyImageView(m_device, view, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void Swapchain::createSwapchain()
{
	auto swapchainDetails = m_swapchainSupportDetails;
	auto surfaceFormat = chooseSwapchainSurfaceFormat(swapchainDetails.formats);
	auto presentMode = chooseSwapchainPresentMode(swapchainDetails.presentModes);
	auto extent = chooseSwapchainExtent(swapchainDetails.capabilities);

	auto imageCount = swapchainDetails.capabilities.minImageCount + 1;
	if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount)
		imageCount = swapchainDetails.capabilities.maxImageCount;

	auto createInfo = VkSwapchainCreateInfoKHR{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = m_queueFamilyIndices;
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

	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan swapchain" };

	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
	m_swapchainFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

VkSurfaceFormatKHR Swapchain::chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
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
		auto actualExtent = m_extent;
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

void Swapchain::createImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());
	for (int i = 0; i < m_swapchainImages.size(); i++)
	{
		auto createInfo = VkImageViewCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]))
			throw std::runtime_error{ "failed to create swapchain image view" };
	}
}

void Swapchain::createFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchainImages.size());
	for (int i = 0; i < m_swapchainImages.size(); i++)
	{
		auto attachments = { m_swapchainImageViews[i] };

		auto createInfo = VkFramebufferCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.pAttachments = attachments.begin();
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.width = m_swapchainExtent.width;
		createInfo.height = m_swapchainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create framebuffer" };
	}
}

void Swapchain::createSyncObjects()
{
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create vulkan sync objects" };
	}
}

void Swapchain::createCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	auto allocInfo = VkCommandBufferAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vulkan command buffers" };
}

VkExtent2D Swapchain::getExtent()
{
	return m_swapchainExtent;
}

VkFormat Swapchain::getFormat()
{
	return m_swapchainFormat;
}

VkSemaphore Swapchain::getImageAvailableSemaphore()
{
	return m_imageAvailableSemaphores[currentFrame];
}

VkSemaphore Swapchain::getRenderFinishedSemaphore()
{
	return m_renderFinishedSemaphores[currentFrame];
}

VkFence Swapchain::getInFlightFence()
{
	return m_inFlightFences[currentFrame];
}

VkFramebuffer Swapchain::getFramebuffer(uint32_t index)
{
	return m_swapchainFramebuffers[index];
}

VkCommandBuffer Swapchain::getCommandBuffer()
{
	return m_commandBuffers[currentFrame];
}

uint32_t Swapchain::getCurrentFrame()
{
	return currentFrame;
}

uint32_t Swapchain::beginFrame()
{
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

	auto imageIndex = uint32_t{};
	vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	return imageIndex;
}

void Swapchain::endFrame(uint32_t imageIndex)
{
	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[currentFrame];
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &imageIndex;
	vkQueuePresentKHR(m_presentQueue, &presentInfo);
}