#pragma once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/device.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <functional>

struct SwapchainProperties
{
	VkRenderPass renderPass;
};

class Swapchain
{
public:
	Swapchain(Device& device, SwapchainProperties properties, std::function<void(uint32_t, uint32_t)> onResize);
	~Swapchain();
	uint32_t beginFrame(VkFence inFlightFence, VkSemaphore imageAvailableSemaphore);
	void endFrame(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore);
	void recreate();
	VkExtent2D getExtent();
	VkFormat getFormat();
	VkFramebuffer getFramebuffer(uint32_t index);
	uint32_t getImageIndex();

private:
	void clear();
	void createSwapchain();
	void createImageViews();
	void createFramebuffers();
	void createDepthResources();

public:
	static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;

private:
	const VkRenderPass m_renderPass;
	Device& m_device;
	std::function<void(uint32_t, uint32_t)> m_onResize;
	uint32_t m_imageIndex;

	VkSwapchainKHR m_swapchain;
	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;

	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
};