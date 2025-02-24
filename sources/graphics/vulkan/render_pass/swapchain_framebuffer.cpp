#include "graphics/vulkan/render_pass/swapchain_framebuffer.hpp"
#include "graphics/vulkan/locator.hpp"
#include "graphics/vulkan/render_pass/render_pass.hpp"

#include <stdexcept>

SwapchainFramebuffer::~SwapchainFramebuffer()
{
	destroy();
}

void SwapchainFramebuffer::destroy()
{
	if (m_initialized)
	{
		m_colorAttachment.destroy();
		m_depthAttachment.destroy();
		vkDestroyFramebuffer(m_device->getDevice(), m_framebuffer, nullptr);
	}
	m_initialized = false;
}

void SwapchainFramebuffer::init(const FramebufferProps& props, VkImage swapchainImage, RenderPass& renderPass, uint32_t width, uint32_t height)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_props = props;
	m_width = width;
	m_height = height;
	m_renderPass = &renderPass;
	createTextures(swapchainImage);
	createFramebuffer();
}

void SwapchainFramebuffer::createTextures(VkImage swapchainImage)
{
	m_colorAttachment.init(swapchainImage, m_props.colorFormat);
	if (m_props.useDepthAttachment)
	{
		m_depthAttachment.init(
			AttachmentType::Depth, m_width, m_height, m_props.depthFormat,
			Locator::getDescriptorPool().createSet(1)
		);
	}
}

void SwapchainFramebuffer::createFramebuffer()
{
	auto imageViews = std::vector<VkImageView>();
	imageViews.reserve(1 + m_props.useDepthAttachment ? 1 : 0);
	imageViews.push_back(m_colorAttachment.getImageView());
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

VkFramebuffer SwapchainFramebuffer::getFramebuffer()
{
	assert(m_initialized);
	return m_framebuffer;
}

VkExtent2D SwapchainFramebuffer::getExtent()
{
	return { m_width, m_height };
}