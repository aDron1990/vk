#pragma once

#include "graphics/vulkan/render_pass/framebuffer_props.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/texture.hpp"

#include <vulkan/vulkan.h>

class RenderPass;
class Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	void init(const FramebufferProps& props, RenderPass& renderPass);
	void destroy();
	void resize(uint32_t newWidth, uint32_t newHeight);

	VkFramebuffer getFramebuffer();
	Texture& getColorTexture(uint32_t id);
	Texture& getDepthTexture();
	VkExtent2D getExtent();

private:
	void createTextures();
	void createFramebuffer();

private:
	Device& m_device;
	RenderPass* m_renderPass{};
	bool m_initialized = false;
	bool m_swapchainFramebuffer = false;

	FramebufferProps m_props{};
	VkFramebuffer m_framebuffer{};
	std::vector<Texture> m_colorAttachments;
	Texture m_depthAttachment;
};