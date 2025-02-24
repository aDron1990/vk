#pragma once

#include "graphics/vulkan/render_pass/framebuffer.hpp"
#include "graphics/vulkan/image/swapchain_image.hpp"

class SwapchainFramebuffer : public Framebuffer
{
public:
	~SwapchainFramebuffer();
	void init(const FramebufferProps& props, VkImage swapchainImage, RenderPass& renderPass, uint32_t width, uint32_t height);
	void destroy();

	VkFramebuffer getFramebuffer() override;
	VkExtent2D getExtent() override;

private:
	void createTextures(VkImage swapchainImage);
	void createFramebuffer();

private:
	bool m_initialized = false;
	Device* m_device;
	RenderPass* m_renderPass{};
	FramebufferProps m_props{};
	uint32_t m_width{};
	uint32_t m_height{};

	VkFramebuffer m_framebuffer{};
	SwapchainImage m_colorAttachment;
	RenderTexture m_depthAttachment;
};