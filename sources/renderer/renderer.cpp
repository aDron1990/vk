#include "renderer/renderer.hpp"
#include "window/window.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <cmrc/cmrc.hpp>

#include <stdexcept>
#include <print>
#include <set>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <unordered_map>

const std::string MODEL_PATH = "resources/models/monkey.obj";
const std::string TEXTURE_PATH = "resources/images/container2.png";

Renderer::Renderer(Window& window) : m_window{window}
{
	createContext();
	createDevice();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createSwapchain();
	createGraphicsPipeline();
	m_specularMap.reset(new Texture{ *m_device, "resources/images/container2_specular.png" });
	m_model.reset(new Model{ *m_device, MODEL_PATH, TEXTURE_PATH });
	m_object.reset(new Object{ *m_device, *m_model });
	m_emiter.reset(new Object{ *m_device, *m_model });
	m_light.reset(new LightBuffer{ *m_device, m_device->getUboFragmentLayout()});
	m_emiterBuffer.reset(new UniformBuffer<Emiter>{ *m_device, m_device->getUboFragmentLayout() });

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
	initInfo.RenderPass = m_combinePass->getRenderPass();
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

	m_emiter->setPosition({ 3.0f, 3.0f, 0.0f });

	test.reset(new Texture{ *m_device, TEXTURE_PATH });
	test2.reset(new Texture{ *m_device, TEXTURE_PATH });
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
	test2.reset();
	m_object.reset();
	m_emiterBuffer.reset();
	m_emiter.reset();
	m_combinePass.reset();
	m_brightPass.reset();
	m_hblurPass.reset();
	m_vblurPass.reset();
	m_renderPass.reset();
	
	m_pipeline.reset();
	m_emitPipeline.reset();
	m_brightPipeline.reset();
	m_hblurPipeline.reset();
	m_vblurPipeline.reset();
	m_combinePipeline.reset();

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
	m_combinePass.reset(new SwapchainPass{ *m_device });
	m_brightPass.reset(new OffscreenPass{ *m_device, 1280, 720 });
	m_hblurPass.reset(new OffscreenPass{ *m_device, 1280, 720 });
	m_vblurPass.reset(new OffscreenPass{ *m_device, 1280, 720 });
	m_renderPass.reset(new OffscreenPass{ *m_device, 1280, 720, 2 });
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	auto createProps = SwapchainProperties{};
	createProps.renderPass = m_combinePass->getRenderPass();
	m_swapchain.reset(new Swapchain{ *m_device, createProps, [&](uint32_t width, uint32_t height) 
		{
			m_renderPass->resize(width, height);
			m_brightPass->resize(width, height);
			m_hblurPass->resize(width, height);
			m_vblurPass->resize(width, height);
		} });
	m_combinePass->setSwapchain(m_swapchain.get());
}

void Renderer::createGraphicsPipeline()
{
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/main/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/main/shader.frag.spv";
		pipelineInfo.renderPass = m_renderPass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getUboVertexLayout(), m_device->getUboFragmentLayout(), m_device->getUboFragmentLayout(), m_device->getSamplerFragmentLayout(), m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		pipelineInfo.attachmentCount = 2;
		m_pipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/emiter/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/emiter/shader.frag.spv";
		pipelineInfo.renderPass = m_renderPass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getUboVertexLayout(), m_device->getUboFragmentLayout(), m_device->getUboFragmentLayout(), m_device->getSamplerFragmentLayout(), m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		pipelineInfo.attachmentCount = 2;
		m_emitPipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/blur/horizontal.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/blur/horizontal.frag.spv";
		pipelineInfo.renderPass = m_hblurPass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		pipelineInfo.attachmentCount = 1;
		m_hblurPipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/blur/vertical.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/blur/vertical.frag.spv";
		pipelineInfo.renderPass = m_vblurPass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		pipelineInfo.attachmentCount = 1;
		m_vblurPipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/pick_bright/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/pick_bright/shader.frag.spv";
		pipelineInfo.renderPass = m_brightPass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		pipelineInfo.attachmentCount = 1;
		m_brightPipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
	{
		auto pipelineInfo = PipelineInfo{};
		pipelineInfo.vertexPath = "resources/shaders/combine/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/combine/shader.frag.spv";
		pipelineInfo.renderPass = m_combinePass->getRenderPass();
		pipelineInfo.descriptorSetLayouts = { m_device->getSamplerFragmentLayout(), m_device->getSamplerFragmentLayout() };
		pipelineInfo.vertexInput = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		pipelineInfo.attachmentCount = 1;
		m_combinePipeline.reset(new Pipeline{ *m_device, pipelineInfo });
	}
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

void Renderer::setViewport(VkCommandBuffer commandBuffer)
{
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
}

void Renderer::renderScene(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline)
{
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration<float, std::chrono::seconds::period>(now - lastTime).count();
	lastTime = now;

	renderPass.begin(commandBuffer);
	setViewport(commandBuffer);

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

	pipeline.bind(commandBuffer);
	m_light->write(light);
	m_light->bind(commandBuffer, pipeline.getLayout(), 1);
	m_specularMap->bind(commandBuffer, pipeline.getLayout(), 4);
	m_object->bindMVP(commandBuffer, pipeline.getLayout(), view, proj);
	m_object->bindTexture(commandBuffer, pipeline.getLayout(), 3);
	m_object->bindMesh(commandBuffer);
	m_object->draw(commandBuffer, pipeline.getLayout());

	m_emitPipeline->bind(commandBuffer);
	m_emiterBuffer->write(Emiter{ .color = {1.0f, 1.0f, 1.0f} });
	m_emiterBuffer->bind(commandBuffer, m_emitPipeline->getLayout(), 1);
	m_emiter->bindMVP(commandBuffer, m_emitPipeline->getLayout(), view, proj);
	m_emiter->bindMesh(commandBuffer);
	m_emiter->draw(commandBuffer, m_emitPipeline->getLayout());

	renderPass.end(commandBuffer);
}

void Renderer::pickBright(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline)
{
	renderPass.begin(commandBuffer);
	setViewport(commandBuffer);
	pipeline.bind(commandBuffer);
	m_renderPass->bindColorImage(commandBuffer, pipeline.getLayout(), 0, 1);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	renderPass.end(commandBuffer);
}

void Renderer::blur(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline, Blur direction)
{
	renderPass.begin(commandBuffer);
	setViewport(commandBuffer);
	pipeline.bind(commandBuffer);
	if (direction == Blur::Horizontal)
		m_brightPass->bindColorImage(commandBuffer, pipeline.getLayout(), 0, 0);
	else
		m_hblurPass->bindColorImage(commandBuffer, pipeline.getLayout(), 0, 0);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	renderPass.end(commandBuffer);
}

void Renderer::combine(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline)
{
	renderPass.begin(commandBuffer);
	setViewport(commandBuffer);
	pipeline.bind(commandBuffer);
	m_renderPass->bindColorImage(commandBuffer, pipeline.getLayout(), 0, 0);
	m_vblurPass->bindColorImage(commandBuffer, pipeline.getLayout(), 1, 0);

	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	renderPass.end(commandBuffer);
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

	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	renderScene(commandBuffer, *m_renderPass, *m_pipeline);
	pickBright(commandBuffer, *m_brightPass, *m_brightPipeline);
	blur(commandBuffer, *m_hblurPass, *m_hblurPipeline, Blur::Horizontal);
	blur(commandBuffer, *m_vblurPass, *m_vblurPipeline, Blur::Vertical);
	combine(commandBuffer, *m_combinePass, *m_combinePipeline);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };


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