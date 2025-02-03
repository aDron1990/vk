#pragma once

#include "renderer/types.hpp"
#include "renderer/device.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

struct SwapchainProperties
{
	VkRenderPass renderPass;
};

class Swapchain
{
public:
	Swapchain(Device& device, SwapchainProperties properties);
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
	uint32_t currentFrame{};

private:
	const VkRenderPass m_renderPass;
	Device& m_device;

	VkQueue m_presentQueue;
	VkSwapchainKHR m_swapchain;

	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
};