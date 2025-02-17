#include "graphics/vulkan/render_pass/offscreen_pass.hpp"
#include "graphics/vulkan/context/swapchain.hpp"
#include "graphics/vulkan/locator.hpp"

#include <stdexcept>

OffscreenPass::OffscreenPass() : m_device{ Locator::getDevice() } {}

OffscreenPass::~OffscreenPass()
{
	destroy();
}

void OffscreenPass::destroy()
{
	if (m_initialized)
	{
		vkDestroyRenderPass(m_device.getDevice(), m_renderPass, nullptr);
	}
	m_initialized = false;
}

void OffscreenPass::init(const FramebufferProps& framebufferProps)
{
	assert(!m_initialized);
	m_framebufferProps = framebufferProps;
	createRenderPass();
	
	m_initialized = true;
}

void OffscreenPass::createRenderPass()
{
	auto details = m_device.querySwapchainSupport(m_device.getGpu());
	auto format = Swapchain::chooseSwapchainSurfaceFormat(details.formats).format;
	auto attachmentsCount = m_framebufferProps.colorAttachmentCount + static_cast<int>(m_framebufferProps.colorAttachmentCount);

	auto attachments = std::vector<VkAttachmentDescription>{};
	attachments.reserve(attachmentsCount);
	auto attachmentRefs = std::vector<VkAttachmentReference>{};
	attachmentRefs.reserve(attachments.size());

	for (size_t i = 0; i < m_framebufferProps.colorAttachmentCount; i++)
	{
		auto colorAttachment = VkAttachmentDescription{};
		colorAttachment.format = format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachments.push_back(colorAttachment);

		auto colorAttachmentRef = VkAttachmentReference{};
		colorAttachmentRef.attachment = i;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentRefs.push_back(colorAttachmentRef);
	}

	auto depthAttachment = VkAttachmentDescription{};
	depthAttachment.format = m_device.findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = m_framebufferProps.useDepthAttachment ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	attachments.push_back(depthAttachment);

	auto depthAttachmentRef = VkAttachmentReference{};
	depthAttachmentRef.attachment = m_framebufferProps.colorAttachmentCount;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = attachmentRefs.data();
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.colorAttachmentCount = attachmentRefs.size();

	std::array<VkSubpassDependency, 2> dependencies{};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstSubpass = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	
	dependencies[1].srcSubpass = 0;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	auto createInfo = VkRenderPassCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = attachments.data();
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = dependencies.data();
	createInfo.dependencyCount = dependencies.size();

	if (vkCreateRenderPass(m_device.getDevice(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan render pass" };
}

void OffscreenPass::begin(VkCommandBuffer commandBuffer, Framebuffer& framebuffer)
{
	auto attachmentsCount = m_framebufferProps.colorAttachmentCount + static_cast<int>(m_framebufferProps.colorAttachmentCount);
	auto clearValues = std::vector<VkClearValue>(attachmentsCount,
			VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}});
	clearValues.back() = VkClearValue{ .depthStencil = {1.0f, 0} };

	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = framebuffer.getFramebuffer();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = framebuffer.getExtent();
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void OffscreenPass::end(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

VkRenderPass OffscreenPass::getRenderPass()
{
	return m_renderPass;
}
