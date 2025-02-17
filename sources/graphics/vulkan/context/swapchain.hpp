#pragma once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/render_pass/framebuffer.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <functional>

class Swapchain
{
public:
	Swapchain(const FramebufferProps& framebufferProps, RenderPass& renderPass, std::function<void(uint32_t, uint32_t)> onResize);
	~Swapchain();
	uint32_t beginFrame(VkFence inFlightFence, VkSemaphore imageAvailableSemaphore);
	void endFrame(uint32_t imageIndex, VkSemaphore renderFinishedSemaphore);
	void recreate();
	VkExtent2D getExtent();
	VkFormat getFormat();
	Framebuffer& getFramebuffer(uint32_t index);
	uint32_t getImageIndex();

private:
	void clear();
	void createSwapchain();
	void createFramebuffers();

public:
	static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;

private:
	RenderPass* m_renderPass{};
	Device& m_device;
	FramebufferProps m_framebufferProps{};
	std::function<void(uint32_t, uint32_t)> m_onResize;
	uint32_t m_imageIndex{};

	VkSwapchainKHR m_swapchain{};
	std::vector<Framebuffer> m_framebuffers{};
};