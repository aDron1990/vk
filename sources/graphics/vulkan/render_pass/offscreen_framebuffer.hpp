#pragma once

#include "graphics/vulkan/render_pass/framebuffer.hpp"

class OffscreenFramebuffer : public Framebuffer
{
public:
	~OffscreenFramebuffer();
	void init(const FramebufferProps& props, RenderPass& renderPass, uint32_t width, uint32_t height);
	void destroy();
	void resize(uint32_t newWidth, uint32_t newHeight);

	VkFramebuffer getFramebuffer() override;
	Texture& getColorTexture(uint32_t id);
	Texture& getDepthTexture();
	VkExtent2D getExtent() override;

private:
	void createTextures();
	void createFramebuffer();

private:
	bool m_initialized = false;
	Device* m_device;
	RenderPass* m_renderPass{};
	FramebufferProps m_props{};
	uint32_t m_width{};
	uint32_t m_height{};

	VkFramebuffer m_framebuffer{};
	std::vector<RenderTexture> m_colorAttachments;
	RenderTexture m_depthAttachment;
};