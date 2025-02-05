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

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

Renderer::Renderer(GLFWwindow* window) : m_window{window}
{
	createContext();
	createDevice();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createTexture();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorSetLayout();
	createDescriptorSets();
	createSwapchain();
	createGraphicsPipeline();

	auto ubo = UniformBuffer<UniformBufferObject>{*m_device};
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device->getDevice());
	m_pipeline.reset();
	m_swapchain.reset();
	for (auto& frameData : m_frameDatas)
	{
		vkDestroySemaphore(m_device->getDevice(), frameData.imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_device->getDevice(), frameData.renderFinishedSemaphore, nullptr);
		vkDestroyFence(m_device->getDevice(), frameData.inFlightFence, nullptr);
	}
	m_uniformBuffer.reset();
	m_texture.reset();
	vkDestroyDescriptorSetLayout(m_device->getDevice(), m_descriptorLayout, nullptr);
	m_vertexBuffer.reset();
	m_indexBuffer.reset();
	vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
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

	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.colorAttachmentCount = 1;

	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	auto createInfo = VkRenderPassCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = &colorAttachment;
	createInfo.attachmentCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = &dependency;
	createInfo.dependencyCount = 1;

	if (vkCreateRenderPass(m_device->getDevice(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan render pass" };
}

void Renderer::createTexture()
{
	m_texture.reset(new Texture{ *m_device, "../../resources/textures/statue.jpg" });
}

void Renderer::createVertexBuffer()
{
	VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

	Buffer stagingBuffer{ *m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	auto* data = stagingBuffer.map();
	memcpy(data, vertices.data(), static_cast<size_t>(size));
	stagingBuffer.unmap();

	m_vertexBuffer.reset(new Buffer{*m_device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT });

	m_device->copyBuffer(stagingBuffer, *m_vertexBuffer);
}

void Renderer::createIndexBuffer()
{
	VkDeviceSize size = sizeof(indices[0]) * indices.size();

	Buffer stagingBuffer{ *m_device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	auto* data = stagingBuffer.map();
	memcpy(data, indices.data(), static_cast<size_t>(size));
	stagingBuffer.unmap();

	m_indexBuffer.reset(new Buffer{ *m_device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT });

	m_device->copyBuffer(stagingBuffer, *m_indexBuffer);
}

void Renderer::createUniformBuffers()
{
	m_uniformBuffer.reset(new UniformBuffer<UniformBufferObject>{ *m_device });
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

void Renderer::createDescriptorSetLayout()
{
	auto uboBind = VkDescriptorSetLayoutBinding{};
	uboBind.binding = 0;
	uboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBind.descriptorCount = 1;
	uboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto samplerBind = VkDescriptorSetLayoutBinding{};
	samplerBind.binding = 1;
	samplerBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBind.descriptorCount = 1;
	samplerBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto bindings = {uboBind, samplerBind};
	auto createInfo = VkDescriptorSetLayoutCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = bindings.begin();
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	if (vkCreateDescriptorSetLayout(m_device->getDevice(), &createInfo, nullptr, &m_descriptorLayout) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor set layout" };
}

void Renderer::createDescriptorSets()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_frameDatas[i].descriptorSet = m_device->allocateDescriptorSets(m_descriptorLayout, 1).back();

		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = m_uniformBuffer->getBuffer(i).getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		auto imageInfo = VkDescriptorImageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_texture->getImageView();
		imageInfo.sampler = m_texture->getSampler();

		auto descriptorWrites = std::vector<VkWriteDescriptorSet>(2);
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_frameDatas[i].descriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].descriptorCount = 1;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_frameDatas[i].descriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].descriptorCount = 1;
		vkUpdateDescriptorSets(m_device->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::createGraphicsPipeline()
{
	auto pipelineInfo = PipelineInfo{};
	pipelineInfo.vertexPath = "../../resources/shaders/shader.vert.spv";
	pipelineInfo.fragmentPath = "../../resources/shaders/shader.frag.spv";
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.descriptorSetLayout = m_descriptorLayout;
	m_pipeline.reset(new Pipeline{ *m_device, pipelineInfo });
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
		frameData.commandBuffer = m_device->allocateCommandBuffers(1).back();
	}
}

void Renderer::updateUniformBuffer()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	auto ubo = UniformBufferObject{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(m_uniformBuffer->getBufferPtr(currentFrame), &ubo, sizeof(ubo));
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapchain->getFramebuffer	(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();
	renderPassInfo.pClearValues = &clearColor;
	renderPassInfo.clearValueCount = 1;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	m_pipeline->bind(commandBuffer);

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

	VkDeviceSize offsets[] = {0};
	auto buffer = m_vertexBuffer->getBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getLayout(), 0, 1, &m_frameDatas[currentFrame].descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

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

	updateUniformBuffer();

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