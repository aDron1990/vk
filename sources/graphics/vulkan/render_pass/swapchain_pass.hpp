#pragma once

#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/swapchain.hpp"

#include <vulkan/vulkan.h>

#include <array>

class SwapchainPass : public RenderPass
{
public:
	SwapchainPass(Device& device);
	~SwapchainPass();
	void begin(VkCommandBuffer commandBuffer, Framebuffer& framebuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	VkRenderPass getRenderPass() override;

private:
	void createRenderPass();

private:
	Device& m_device;
	VkRenderPass m_renderPass;
};