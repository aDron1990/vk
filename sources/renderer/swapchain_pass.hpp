#pragma once

#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"

#include <vulkan/vulkan.h>

#include <array>

class SwapchainPass
{
public:
	SwapchainPass(Device& device);
	~SwapchainPass();
	void begin(VkCommandBuffer commandBuffer, const std::array<VkClearValue, 2>& clearValues, uint32_t imageIndex);
	void end(VkCommandBuffer commandBuffer);
	void setSwapchain(Swapchain* swapchain);
	VkRenderPass getRenderPass();

private:
	void createRenderPass();

private:
	Device& m_device;
	Swapchain* m_swapchain;
	VkRenderPass m_renderPass;

};