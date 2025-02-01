#pragma once

#include "renderer/types.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

struct SwapchainProperties
{
	VkInstance instance;
	VkPhysicalDevice gpu;
	VkDevice device;
	VkCommandPool commandPool;
	GLFWwindow* window;
	FindQueueFamilyFunc findQueueFamily;
};

class Swapchain
{
public:
	Swapchain(SwapchainProperties properties);
	~Swapchain();

	VkExtent2D getExtent();
	VkRenderPass getRenderPass();
	VkSemaphore getImageAvailableSemaphore();
	VkSemaphore getRenderFinishedSemaphore();
	VkCommandBuffer getCommandBuffer();
	VkFence getInFlightFence();
	VkFramebuffer getFramebuffer(uint32_t index);

	uint32_t getCurrentFrame();

	uint32_t beginFrame();
	void endFrame(uint32_t imageIndex);

private:
	void createSurface();
	void createSwapchain();
	void createImageViews();
	void createRenderPass();
	void createFramebuffers();
	void createSyncObjects();
	void createCommandBuffers();

private:
	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

private:
	FindQueueFamilyFunc findQueueFamilies;
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);
	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;

private:
	const VkInstance m_instance;
	const VkPhysicalDevice m_gpu;
	const VkDevice m_device;
	const VkCommandPool m_commandPool;
	GLFWwindow* m_window;
	uint32_t currentFrame{};

	VkQueue m_presentQueue;
	VkRenderPass m_renderPass;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;

	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
};