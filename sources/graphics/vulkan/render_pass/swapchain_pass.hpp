#pragma once

#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/context/swapchain.hpp"

#include <vulkan/vulkan.h>

#include <array>

class SwapchainPass : public RenderPass
{
public:
	~SwapchainPass();
	void init();
	void destroy();

	void begin(VkCommandBuffer commandBuffer, Framebuffer& framebuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	VkRenderPass getRenderPass() override;

private:
	void createRenderPass();

private:
	bool m_initialized = false;
	Device* m_device{};
	VkRenderPass m_renderPass{};
};