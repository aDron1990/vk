#pragma once

#include "renderer/types.hpp"
#include "renderer/context.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

struct SwapchainProperties
{
	VkPhysicalDevice gpu;
	VkDevice device;
	VkRenderPass renderPass;
	VkSurfaceKHR surface;
	VkExtent2D extent;
	QueueFamilyIndices queueFamilyIndices;
	SwapchainSupportDetails swapchainSupportDetails;
};

class Swapchain
{
public:
	Swapchain(SwapchainProperties properties);
	~Swapchain();

	VkExtent2D getExtent();
	VkFormat getFormat();
	VkFramebuffer getFramebuffer(uint32_t index);

	uint32_t getCurrentFrame();

	uint32_t beginFrame(VkFence inFlightFence, VkSemaphore imageAvailableSemaphore);
	void endFrame(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore);

private:
	void createSwapchain();
	void createImageViews();
	void createFramebuffers();

public:
	static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	VkExtent2D m_extent;
	uint32_t currentFrame{};

private:
	const VkPhysicalDevice m_gpu;
	const VkDevice m_device;
	const VkRenderPass m_renderPass;
	const VkSurfaceKHR m_surface;
	const QueueFamilyIndices m_queueFamilyIndices;
	const SwapchainSupportDetails m_swapchainSupportDetails;
	
	VkQueue m_presentQueue;
	VkSwapchainKHR m_swapchain;

	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
};