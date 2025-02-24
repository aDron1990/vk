#include "graphics/vulkan/render_pass/offscreen_framebuffer.hpp"
#include "graphics/vulkan/locator.hpp"
#include "graphics/vulkan/render_pass/render_pass.hpp"

#include <stdexcept>

OffscreenFramebuffer::~OffscreenFramebuffer()
{
	destroy();
}

void OffscreenFramebuffer::destroy()
{
	if (m_initialized)
	{
		for (auto& colorAttachment : m_colorAttachments)
		{
			colorAttachment.destroy();
		}
		m_depthAttachment.destroy();
		vkDestroyFramebuffer(m_device->getDevice(), m_framebuffer, nullptr);
	}
	m_initialized = false;
}

void OffscreenFramebuffer::init(const FramebufferProps& props, RenderPass& renderPass, uint32_t width, uint32_t height)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_props = props;
	m_width = width;
	m_height = height;
	m_renderPass = &renderPass;
	createTextures();
	createFramebuffer();
}

void OffscreenFramebuffer::resize(uint32_t newWidth, uint32_t newHeight)
{
	destroy();
	init(m_props, *m_renderPass, newWidth, newHeight);
}

void OffscreenFramebuffer::createTextures()
{
	m_colorAttachments.resize(m_props.colorAttachmentCount);
	for (auto& colorAttachment : m_colorAttachments)
	{
		colorAttachment.init(
			AttachmentType::Color, m_width, m_height, m_props.colorFormat,
			Locator::getDescriptorPool().createSet(1)
		);
	}
	if (m_props.useDepthAttachment)
	{
		m_depthAttachment.init(
			AttachmentType::Depth, m_width, m_height, m_props.depthFormat,
			Locator::getDescriptorPool().createSet(1)
		);
	}
}

void OffscreenFramebuffer::createFramebuffer()
{
	auto imageViews = std::vector<VkImageView>();
	imageViews.reserve(m_colorAttachments.size()
		+ m_props.useDepthAttachment ? 1 : 0
	);
	for (auto& colorImage : m_colorAttachments)
	{
		imageViews.push_back(colorImage.getImageView());
	}
	if (m_props.useDepthAttachment)
		imageViews.push_back(m_depthAttachment.getImageView());

	auto createInfo = VkFramebufferCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = m_renderPass->getRenderPass();
	createInfo.pAttachments = imageViews.data();
	createInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
	createInfo.width = m_width;
	createInfo.height = m_height;
	createInfo.layers = 1;
	if (vkCreateFramebuffer(m_device->getDevice(), &createInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create framebuffer" };
}

VkFramebuffer OffscreenFramebuffer::getFramebuffer()
{
	assert(m_initialized);
	return m_framebuffer;
}

Texture& OffscreenFramebuffer::getColorTexture(uint32_t id)
{
	assert(m_initialized);
	return m_colorAttachments[id];
}

Texture& OffscreenFramebuffer::getDepthTexture()
{
	assert(m_initialized);
	return m_depthAttachment;
}

VkExtent2D OffscreenFramebuffer::getExtent()
{
	return { m_width, m_height };
}