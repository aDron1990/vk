#pragma once

#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/render_pass/framebuffer_props.hpp"
#include "graphics/vulkan/render_pass/framebuffer.hpp"
#include "graphics/vulkan/context/device.hpp"

#include <vector>

class OffscreenPass : public RenderPass
{
public:
	~OffscreenPass();
	void init(const FramebufferProps& framebufferProps);
	void destroy();

	void begin(VkCommandBuffer commandBuffer, Framebuffer& framebuffer) override;
	void end(VkCommandBuffer commandBuffer) override;
	VkRenderPass getRenderPass() override;

private:
	void createRenderPass();

private:
	bool m_initialized = false;
	Device* m_device{};
	FramebufferProps m_framebufferProps{};
	VkRenderPass m_renderPass{};
};