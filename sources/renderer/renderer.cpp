#include "renderer/renderer.hpp"
#include "load_file.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <print>
#include <set>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <unordered_map>

const std::string MODEL_PATH = "../../resources/models/viking_room.obj";

Renderer::Renderer(GLFWwindow* window) : m_window{window}
{
	createContext();
	createDevice();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createTexture();
	createMesh();
	createSwapchain();
	createGraphicsPipeline();
	m_model.reset(new Model{ *m_device, m_mesh, m_texture });
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device->getDevice());
	for (auto& frameData : m_frameDatas)
	{
		vkDestroySemaphore(m_device->getDevice(), frameData.imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_device->getDevice(), frameData.renderFinishedSemaphore, nullptr);
		vkDestroyFence(m_device->getDevice(), frameData.inFlightFence, nullptr);
	}
	m_model.reset();
	vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
	m_pipeline.reset();
	m_swapchain.reset();
	m_texture.reset();
	m_mesh.reset();
	m_device.reset();
	m_context.reset();
}

void Renderer::createContext()
{
	m_context.reset(new Context{});
}

void Renderer::createDevice()
{
	auto surface = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(m_context->getInstance(), m_window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan surface" };

	m_device.reset(new Device{*m_context, surface});
}

void Renderer::createRenderPass()
{
	auto details = m_device->querySwapchainSupport(m_device->getGpu());
	auto format = Swapchain::chooseSwapchainSurfaceFormat(details.formats);
	auto colorAttachment = VkAttachmentDescription{};
	colorAttachment.format = format.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	auto colorAttachmentRef = VkAttachmentReference{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	auto depthAttachment = VkAttachmentDescription{};
	depthAttachment.format = m_device->findDepthFormat();
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

	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	auto attachments = { colorAttachment, depthAttachment };
	auto createInfo = VkRenderPassCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = attachments.begin();
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = &dependency;
	createInfo.dependencyCount = 1;

	if (vkCreateRenderPass(m_device->getDevice(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan render pass" };
}

void Renderer::createTexture()
{
	m_texture.reset(new Texture{ *m_device, "../../resources/textures/viking_room.png" });
}

void Renderer::createMesh()
{
	m_mesh.reset(new Mesh{ *m_device, "MODEL_PATH" });
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	auto createProps = SwapchainProperties{};
	createProps.renderPass = m_renderPass;
	m_swapchain.reset(new Swapchain{*m_device, createProps});
}

void Renderer::createGraphicsPipeline()
{
	auto pipelineInfo = PipelineInfo{};
	pipelineInfo.vertexPath = "../../resources/shaders/shader.vert.spv";
	pipelineInfo.fragmentPath = "../../resources/shaders/shader.frag.spv";
	pipelineInfo.renderPass = m_renderPass;
	m_pipeline.reset(new GraphicsPipeline{ *m_device, pipelineInfo });
}

void Renderer::createSyncObjects()
{
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (auto& frameData : m_frameDatas)
	{
		if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &frameData.imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &frameData.renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &frameData.inFlightFence) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create vulkan sync objects" };
	}
}

void Renderer::createCommandBuffers()
{
	for (auto& frameData : m_frameDatas)
	{
		frameData.commandBuffer = m_device->createCommandBuffers(1).back();
	}
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearValues = 
	{ 
		VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
		VkClearValue{.depthStencil = {1.0f, 0}}
	};
	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapchain->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();
	renderPassInfo.pClearValues = clearValues.begin();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	m_pipeline->bind(commandBuffer, currentFrame);

	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain->getExtent().width);
	viewport.height = static_cast<float>(m_swapchain->getExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain->getExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	auto extent = m_swapchain->getExtent();
	auto ubo = UniformBufferObject{};
	auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
	proj[1][1] *= -1;

	m_model->draw(commandBuffer, m_pipeline->getLayout(), currentFrame, view, proj);

	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };
}

void Renderer::draw()
{
	auto imageIndex = m_swapchain->beginFrame(m_frameDatas[currentFrame].inFlightFence, m_frameDatas[currentFrame].imageAvailableSemaphore);
	if (imageIndex == UINT32_MAX) return;
	auto commandBuffer = m_frameDatas[currentFrame].commandBuffer;

	vkResetCommandBuffer(commandBuffer, 0);

	recordCommandBuffer(commandBuffer, imageIndex);

	std::initializer_list<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	auto submitInfo = VkSubmitInfo{};
	auto signalSemaphores = { m_frameDatas[currentFrame].renderFinishedSemaphore };
	auto waitSemaphores = { m_frameDatas[currentFrame].imageAvailableSemaphore };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitSemaphores = waitSemaphores.begin();
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitDstStageMask = waitStages.begin();
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.begin();
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_frameDatas[currentFrame].inFlightFence) != VK_SUCCESS)
		throw std::runtime_error{ "failed to submit draw command buffer" };

	m_swapchain->endFrame(imageIndex, m_frameDatas[currentFrame].renderFinishedSemaphore);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}