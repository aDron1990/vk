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

const std::string MODEL_PATH = "../../resources/models/monkey.obj";
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
	m_object.reset(new Object{ *m_device, *m_model });
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
	initInfo.RenderPass = m_swapchainPass->getRenderPass();
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = 3;
	initInfo.DescriptorPoolSize = 128;
	if (!ImGui_ImplVulkan_Init(&initInfo))
		throw;
	
	ImGui_ImplVulkan_CreateFontsTexture();



	light.direction = { -0.2f, -1.0f, -0.3f };
	light.ambient = { 0.2f, 0.2f, 0.2f };
	light.diffuse = { 0.5f, 0.5f, 0.5f };
	light.specular = { 1.0f, 1.0f, 1.0f };

	test.reset(new Texture{ *m_device, TEXTURE_PATH });
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device->getDevice());

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vkDestroySemaphore(m_device->getDevice(), m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_device->getDevice(), m_renderFinishedSemaphore, nullptr);
	vkDestroyFence(m_device->getDevice(), m_inFlightFence, nullptr);

	test.reset();
	m_object.reset();
	m_swapchainPass.reset();
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
	m_swapchainPass.reset(new SwapchainPass{*m_device});
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	auto createProps = SwapchainProperties{};
	createProps.renderPass = m_swapchainPass->getRenderPass();
	m_swapchain.reset(new Swapchain{*m_device, createProps});
	m_swapchainPass->setSwapchain(m_swapchain.get());
}

void Renderer::createGraphicsPipeline()
{
	auto pipelineInfo = PipelineInfo{};
	pipelineInfo.vertexPath = "../../resources/shaders/post/shader.vert.spv";
	pipelineInfo.fragmentPath = "../../resources/shaders/post/shader.frag.spv";
	pipelineInfo.renderPass = m_swapchainPass->getRenderPass();
	//pipelineInfo.descriptorSetLayouts = { m_device->getMVPLayout(), m_device->getLightLayout(), m_device->getMaterialLayout(), m_device->getSamplerLayout(), m_device->getSamplerLayout() };
	pipelineInfo.descriptorSetLayouts = { m_device->getSamplerLayout() };
	pipelineInfo.vertexInput = false;
	pipelineInfo.culling = VK_CULL_MODE_NONE;
	m_pipeline.reset(new Pipeline{ *m_device, pipelineInfo });
}

void Renderer::createSyncObjects()
{
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan sync objects" };
}

void Renderer::createCommandBuffers()
{
	m_commandBuffer = m_device->createCommandBuffers(1).back();
}

void Renderer::renderScene(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearValues = 
	std::array<VkClearValue, 2>{
		VkClearValue{.color = {{0.05f, 0.05f, 0.05f, 1.0f}}},
		VkClearValue{.depthStencil = {1.0f, 0}}
	};
	m_swapchainPass->begin(commandBuffer, clearValues, imageIndex);

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

#if 0
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
	auto proj = glm::perspective(glm::radians(80.0f), extent.width / (float)extent.height, 0.1f, 100.0f);
	proj[1][1] *= -1;

	light.viewPosition = m_camera.getPosition();

	m_light->write(light);
	m_light->bind(commandBuffer, m_pipeline->getLayout(), 1);
	m_specularMap->bind(commandBuffer, m_pipeline->getLayout(), 4);
	m_object->draw(commandBuffer, m_pipeline->getLayout(), view, proj);
#endif
	test->bind(commandBuffer, m_pipeline->getLayout(), 0);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	m_swapchainPass->end(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };
}

void Renderer::render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	auto pos = m_object->getPosition();
	auto cachePos = m_object->getPosition();

	ImGui::Begin("Object");
	ImGui::DragFloat3("position", &pos.x, 0.1f);
	ImGui::Separator();
	ImGui::DragFloat("shininess", &m_object->material.shininess, 0.5f, 0.5f, 128.0f);
	ImGui::End();

	if (pos != cachePos) m_object->setPosition(pos);
	
	ImGui::Begin("Light");
	ImGui::DragFloat3("direction", (float*)&light.direction);
	ImGui::ColorEdit3("ambient", (float*)&light.ambient);
	ImGui::ColorEdit3("diffuse", (float*)&light.diffuse);
	ImGui::ColorEdit3("specular", (float*)&light.specular);
	ImGui::End();

	ImGui::Render();

	auto imageIndex = m_swapchain->beginFrame(m_inFlightFence, m_imageAvailableSemaphore);
	if (imageIndex == UINT32_MAX) return;
	auto commandBuffer = m_commandBuffer;

	vkResetCommandBuffer(commandBuffer, 0);

	renderScene(commandBuffer, imageIndex);

	std::initializer_list<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	auto submitInfo = VkSubmitInfo{};
	auto signalSemaphores = { m_renderFinishedSemaphore };
	auto waitSemaphores = { m_imageAvailableSemaphore };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitSemaphores = waitSemaphores.begin();
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitDstStageMask = waitStages.begin();
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.begin();
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_inFlightFence) != VK_SUCCESS)
		throw std::runtime_error{ "failed to submit draw command buffer" };

	m_swapchain->endFrame(imageIndex, m_renderFinishedSemaphore);
}