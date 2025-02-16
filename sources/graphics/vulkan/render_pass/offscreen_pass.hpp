#pragma once

#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/render_pass/framebuffer_props.hpp"
#include "graphics/vulkan/render_pass/framebuffer.hpp"
#include "graphics/vulkan/device.hpp"

#include <vector>

class OffscreenPass : public RenderPass
{
public:
	OffscreenPass();
	~OffscreenPass();
	void init(const FramebufferProps& framebufferProps);
	void destroy();

	Framebuffer createFramebuffer(uint32_t width, uint32_t height);
	void begin(VkCommandBuffer commandBuffer, Framebuffer* framebuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	VkRenderPass getRenderPass() override;

private:
	void createRenderPass();

private:
	Device& m_device;
	bool m_initialized = false;
	FramebufferProps m_framebufferProps{};
	VkRenderPass m_renderPass{};
};