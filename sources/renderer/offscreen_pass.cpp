#include "renderer/offscreen_pass.hpp"
#include "renderer/swapchain.hpp"


#include <stdexcept>

OffscreenPass::OffscreenPass(Device& device, uint32_t width, uint32_t height) : m_device{ device }, m_width{width}, m_height{height}
{
	createRenderPass();
	createImages();
}

OffscreenPass::~OffscreenPass()
{
	clearImages();
	vkDestroyRenderPass(m_device.getDevice(), m_renderPass, nullptr);
}

void OffscreenPass::createRenderPass()
{
	auto details = m_device.querySwapchainSupport(m_device.getGpu());
	auto format = Swapchain::chooseSwapchainSurfaceFormat(details.formats);

	auto colorAttachment = VkAttachmentDescription{};
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	auto colorAttachmentRef = VkAttachmentReference{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	auto depthAttachment = VkAttachmentDescription{};
	depthAttachment.format = m_device.findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	auto depthAttachmentRef = VkAttachmentReference{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.colorAttachmentCount = 1;

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

	auto attachments = { colorAttachment, depthAttachment };
	auto createInfo = VkRenderPassCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = attachments.begin();
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = dependencies.data();
	createInfo.dependencyCount = dependencies.size();

	if (vkCreateRenderPass(m_device.getDevice(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan render pass" };
}

void OffscreenPass::clearImages()
{
	m_descriptorSet.free();
	vkDestroySampler(m_device.getDevice(), m_sampler, nullptr);
	vkDestroyImageView(m_device.getDevice(), m_imageView, nullptr);
	vkDestroyImage(m_device.getDevice(), m_image, nullptr);
	vkFreeMemory(m_device.getDevice(), m_imageMemory, nullptr);
	vkDestroyImageView(m_device.getDevice(), m_depthImageView, nullptr);
	vkDestroyImage(m_device.getDevice(), m_depthImage, nullptr);
	vkFreeMemory(m_device.getDevice(), m_depthImageMemory, nullptr);
	vkDestroyFramebuffer(m_device.getDevice(), m_framebuffer, nullptr);
}

void OffscreenPass::createImages()
{
	createImage();
	createDepthImage();
	createFramebuffer();
	createSampler();
	createDescriptorSet();
}

void OffscreenPass::createImage()
{
	auto details = m_device.querySwapchainSupport(m_device.getGpu());
	auto format = Swapchain::chooseSwapchainSurfaceFormat(details.formats).format;
	m_device.createImage(m_width, m_height, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);
	m_imageView = m_device.createImageView(m_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	m_device.transitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
}

void OffscreenPass::createDepthImage()
{
	auto depthFormat = m_device.findDepthFormat();
	m_device.createImage(m_width, m_height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
	m_depthImageView = m_device.createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	m_device.transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void OffscreenPass::createFramebuffer()
{
	auto attachments = { m_imageView, m_depthImageView };
	auto createInfo = VkFramebufferCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = m_renderPass;
	createInfo.pAttachments = attachments.begin();
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.width = m_width;
	createInfo.height = m_height;
	createInfo.layers = 1;
	if (vkCreateFramebuffer(m_device.getDevice(), &createInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create framebuffer" };
}

void OffscreenPass::createSampler()
{
	auto gpuProps = VkPhysicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(m_device.getGpu(), &gpuProps);

	auto createInfo = VkSamplerCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = gpuProps.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 1;
	if (vkCreateSampler(m_device.getDevice(), &createInfo, nullptr, &m_sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

void OffscreenPass::createDescriptorSet()
{
	m_descriptorSet = m_device.createDescriptorSet(m_device.getSamplerFragmentLayout());

	auto samplerInfo = VkDescriptorImageInfo{};
	samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	samplerInfo.imageView = m_imageView;
	samplerInfo.sampler = m_sampler;

	auto descriptorWrite = VkWriteDescriptorSet{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_descriptorSet.set;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.pImageInfo = &samplerInfo;
	descriptorWrite.descriptorCount = 1;
	vkUpdateDescriptorSets(m_device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void OffscreenPass::begin(VkCommandBuffer commandBuffer, const std::array<VkClearValue, 2>& clearValues)
{
	auto extent = VkExtent2D{};
	extent.width = m_width;
	extent.height = m_height;
	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.pClearValues = clearValues.data();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void OffscreenPass::end(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void OffscreenPass::bindDescriptorSet(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set, 1, &m_descriptorSet.set, 0, nullptr);
}

void OffscreenPass::resize(uint32_t newWidth, uint32_t newHeight)
{
	clearImages();
	m_width = newWidth;
	m_height = newHeight;
	createImages();
}

VkRenderPass OffscreenPass::getRenderPass()
{
	return m_renderPass;
}
