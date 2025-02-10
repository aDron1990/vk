#include "renderer/renderer.hpp"
#include "window/window.hpp"
#include "load_file.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <stdexcept>
#include <print>
#include <set>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <unordered_map>

const std::string MODEL_PATH = "../../resources/models/plane.obj";
const std::string TEXTURE_PATH = "../../resources/textures/container2.png";

Renderer::Renderer(Window& window) : m_window{window}
{
	createContext();
	createDevice();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createSwapchain();
	createGraphicsPipeline();
	m_specularMap.reset(new Texture{ *m_device, "../../resources/textures/container2_specular.png" });
	m_model.reset(new Model{ *m_device, MODEL_PATH, TEXTURE_PATH });
	m_sphere.reset(new Object{ *m_device, *m_model });
	m_light.reset(new LightBuffer{ *m_device, m_device->getLightLayout()});


	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::SetNavCursorVisible(false);
	ImGui_ImplGlfw_InitForVulkan(window.getWindow(), true);

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = m_context->getInstance();
	initInfo.PhysicalDevice = m_device->getGpu();
	initInfo.Device = m_device->getDevice();
	initInfo.QueueFamily = m_device->findQueueFamilies(m_device->getGpu()).graphics.value();
	initInfo.Queue = m_device->getGraphicsQueue();
	initInfo.RenderPass = m_renderPass;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = 3;
	initInfo.DescriptorPoolSize = 128;
	if (!ImGui_ImplVulkan_Init(&initInfo))
		throw;
	
	ImGui_ImplVulkan_CreateFontsTexture();



	light.position = { 6.0f, 5.0f, 10.0f };
	light.ambient = { 0.2f, 0.2f, 0.2f };
	light.diffuse = { 0.5f, 0.5f, 0.5f };
	light.specular = { 1.0f, 1.0f, 1.0f };
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device->getDevice());

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	
	for (auto& frameData : m_frameDatas)
	{
		vkDestroySemaphore(m_device->getDevice(), frameData.imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_device->getDevice(), frameData.renderFinishedSemaphore, nullptr);
		vkDestroyFence(m_device->getDevice(), frameData.inFlightFence, nullptr);
	}
	m_sphere.reset();
	vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
	m_pipeline.reset();
	m_swapchain.reset();
	m_specularMap.reset();
	m_model.reset();
	m_light.reset();
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
	if (glfwCreateWindowSurface(m_context->getInstance(), m_window.getWindow(), nullptr, &surface) != VK_SUCCESS)
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

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
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

void Renderer::renderScene(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearValues = 
	{ 
		VkClearValue{.color = {{0.05f, 0.05f, 0.05f, 1.0f}}},
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

	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration<float, std::chrono::seconds::period>(now - lastTime).count();
	lastTime = now;

	static auto& input = m_window.getInput();
	auto cameraMove = glm::vec3{};
	if (input.getKeyDown(GLFW_KEY_Q))
		input.lockCursor(!input.getCursorLock());
	if (input.getCursorLock())
	{
		m_camera.rotate(input.getCursorDelta(), delta);
	}

	if (input.getKey('W')) cameraMove.z += 1;
	if (input.getKey('S')) cameraMove.z -= 1;
	if (input.getKey('D')) cameraMove.x += 1;
	if (input.getKey('A')) cameraMove.x -= 1;
	if (input.getKey(GLFW_KEY_SPACE)) cameraMove.y += 1;
	if (input.getKey(GLFW_KEY_LEFT_SHIFT)) cameraMove.y -= 1;
	m_camera.move(cameraMove, delta);

	auto extent = m_swapchain->getExtent();
	auto view = m_camera.getViewMatrix();
	auto proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 100.0f);
	proj[1][1] *= -1;

	light.viewPosition = m_camera.getPosition();

	

	m_light->write(light, currentFrame);
	m_light->bind(commandBuffer, m_pipeline->getLayout(), 1, currentFrame);
	m_specularMap->bind(commandBuffer, m_pipeline->getLayout(), 4, currentFrame);
	m_sphere->draw(commandBuffer, m_pipeline->getLayout(), currentFrame, view, proj);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };
}

void Renderer::render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Begin("Object");
	ImGui::SliderFloat("shininess", &m_sphere->material.shininess, 0.0f, 128);
	ImGui::End();
	
	ImGui::Begin("Light");
	ImGui::ColorEdit3("ambient", (float*)&light.ambient);
	ImGui::ColorEdit3("diffuse", (float*)&light.diffuse);
	ImGui::ColorEdit3("specular", (float*)&light.specular);
	ImGui::End();

	ImGui::Render();



	auto imageIndex = m_swapchain->beginFrame(m_frameDatas[currentFrame].inFlightFence, m_frameDatas[currentFrame].imageAvailableSemaphore);
	if (imageIndex == UINT32_MAX) return;
	auto commandBuffer = m_frameDatas[currentFrame].commandBuffer;

	vkResetCommandBuffer(commandBuffer, 0);

	renderScene(commandBuffer, imageIndex);

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