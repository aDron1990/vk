#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "graphics/vulkan/renderer.hpp"
#include "graphics/vulkan/render_pass/framebuffer.hpp"
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

#define TRACY_ENABLE
#include <tracy/Tracy.hpp>

Renderer::Renderer(Window& window) : m_window{window}
{
	createContext();
	createDevice();
	createDescriptorPool();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createSwapchain();
	createGraphicsPipeline();


	m_model.init(MODEL_PATH, TEXTURE_PATH);
	m_vpBuffer.init(m_descriptorPool.createSet(0));


	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::SetNavCursorVisible(false);
	ImGui_ImplGlfw_InitForVulkan(window.getWindow(), true);
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = m_context.getInstance();
	initInfo.PhysicalDevice = m_device.getGpu();
	initInfo.Device = m_device.getDevice();
	initInfo.QueueFamily = m_device.findQueueFamilies(m_device.getGpu()).graphics.value();
	initInfo.Queue = m_device.getGraphicsQueue();
	initInfo.RenderPass = m_renderPass.getRenderPass();
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = 3;
	initInfo.DescriptorPoolSize = 128;
	if (!ImGui_ImplVulkan_Init(&initInfo))
		throw;
	ImGui_ImplVulkan_CreateFontsTexture();
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device.getDevice());

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vkDestroySemaphore(m_device.getDevice(), m_imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_device.getDevice(), m_renderFinishedSemaphore, nullptr);
	vkDestroyFence(m_device.getDevice(), m_inFlightFence, nullptr);
}

void Renderer::createContext()
{
	m_context.init();
}

void Renderer::createDevice()
{
	auto surface = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(m_context.getInstance(), m_window.getWindow(), nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan surface" };

	m_device.init(surface);
}

void Renderer::createDescriptorPool()
{
	auto props = DescriptorPoolProps{};
	props.setInfos =
	{
		DescriptorSetInfo
		{{
			BindingInfo
			{
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			}
		}, VK_SHADER_STAGE_ALL_GRAPHICS, 100 },
		DescriptorSetInfo
		{{
			BindingInfo
			{
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			}
		}, VK_SHADER_STAGE_ALL_GRAPHICS, 100 }
	};
	m_descriptorPool.init(props);
}

void Renderer::createRenderPass()
{
	m_renderPass.init();

	m_renderFramebufferProps.colorAttachmentCount = 1;
	m_renderFramebufferProps.useDepthAttachment = true;
	m_renderFramebufferProps.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	m_renderFramebufferProps.depthFormat = VK_FORMAT_D32_SFLOAT;
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	m_swapchain.init(m_renderFramebufferProps, m_renderPass, [&](uint32_t width, uint32_t height)
	{

	});
}

void Renderer::createGraphicsPipeline()
{
	{
		auto pipelineInfo = PipelineProps{};
		pipelineInfo.vertexPath = "resources/shaders/test/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/test/shader.frag.spv";
		pipelineInfo.descriptorSetLayouts = { m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(1) };
		pipelineInfo.vertexInput = true;
		pipelineInfo.usePushConstants = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		m_renderPipeline.init(pipelineInfo, m_renderFramebufferProps, m_renderPass);
	}
}

void Renderer::createSyncObjects()
{
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(m_device.getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(m_device.getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(m_device.getDevice(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan sync objects" };
}

void Renderer::createCommandBuffers()
{
	m_commandBuffer = m_device.createCommandBuffers(1).back();
}

void Renderer::setViewport(VkCommandBuffer commandBuffer)
{
	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain.getExtent().width);
	viewport.height = static_cast<float>(m_swapchain.getExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain.getExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::setViewport(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
{
	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(width);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = { width, height };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::renderScene(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline, uint32_t imageIndex)
{
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration<float, std::chrono::seconds::period>(now - lastTime).count();
	lastTime = now;

	renderPass.begin(commandBuffer, m_swapchain.getFramebuffer(imageIndex));
	setViewport(commandBuffer);
	pipeline.bind(commandBuffer);

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
	auto extent = m_swapchain.getExtent();

	auto view = m_camera.getViewMatrix();
	auto proj = glm::perspective(glm::radians(80.0f), extent.width / (float)extent.height, 0.1f, 100.0f);
	proj[1][1] *= -1;

	auto vp = ViewProjection{};
	vp.view = view;
	vp.proj = proj;
	m_vpBuffer.write(vp);
	m_vpBuffer.bind(commandBuffer, pipeline.getLayout(), 0);

	alignas(16) auto model = glm::translate(glm::mat4{ 1.0f }, {0.0f, 3.0f, 0.0f});
	vkCmdPushConstants(commandBuffer, pipeline.getLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(model), &model);

	m_model.bindTexture(commandBuffer, pipeline.getLayout(), 1);
	m_model.bindMesh(commandBuffer);
	m_model.draw(commandBuffer, pipeline.getLayout());

	renderPass.end(commandBuffer);
}

void Renderer::render()
{
	ZoneScopedN("render");

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Render();
	
	uint32_t imageIndex;
	imageIndex = m_swapchain.beginFrame(m_inFlightFence, m_imageAvailableSemaphore);
	if (imageIndex == UINT32_MAX) return;

	auto commandBuffer = m_commandBuffer;
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	renderScene(commandBuffer, m_renderPass, m_renderPipeline, imageIndex);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };

	std::initializer_list<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
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
	if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_inFlightFence) != VK_SUCCESS)
		throw std::runtime_error{ "failed to submit draw command buffer" };

	m_swapchain.endFrame(imageIndex, m_renderFinishedSemaphore);
}