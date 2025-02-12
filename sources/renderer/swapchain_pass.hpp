#pragma once

#include "renderer/render_pass.hpp"
#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"

#include <vulkan/vulkan.h>

#include <array>

class SwapchainPass : public RenderPass
{
public:
	SwapchainPass(Device& device);
	~SwapchainPass();
	void begin(VkCommandBuffer commandBuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	void setSwapchain(Swapchain* swapchain);
	VkRenderPass getRenderPass();

private:
	void createRenderPass();

private:
	Device& m_device;
	Swapchain* m_swapchain;
	VkRenderPass m_renderPass;
};