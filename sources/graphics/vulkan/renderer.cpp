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

	m_specularMap.init("resources/images/container2_specular.png", m_descriptorPool.createSet(1));
	m_planeSpecularMap.init("resources/images/brown_specular.png", m_descriptorPool.createSet(1));

	m_skybox.init("resources/images/skybox", m_descriptorPool.createSet(1));

	m_model.init(MODEL_PATH, TEXTURE_PATH);
	m_cube.init("resources/models/cube.obj", "resources/images/brown.png");
	m_planeModel.init("resources/models/plane.obj", "resources/images/brown.png");
	m_object.init(m_model);
	m_plane.init(m_planeModel);
	m_skyboxCube.init(m_cube);
	m_skyboxMvp.init(m_descriptorPool.createSet(0));

	m_object.setPosition({ 0.f, 1.f, 0.f });
	m_plane.setPosition({0.f, .0f, 0.f});

	m_light.init(m_descriptorPool.createSet(0));
	light.direction = { -0.2f, -1.0f, -0.3f };
	light.ambient = { 0.2f, 0.2f, 0.2f };
	light.diffuse = { 0.5f, 0.5f, 0.5f };
	light.specular = { 1.0f, 1.0f, 1.0f };

	m_global.gamma = 2.2f;

	m_shadowMvp.init(m_descriptorPool.createSet(0));
	m_shadowMvp2.init(m_descriptorPool.createSet(0));
	m_globalBuffer.init(m_descriptorPool.createSet(0));
	m_lightSpace.init(m_descriptorPool.createSet(0));

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
	initInfo.RenderPass = m_swapchainPass.getRenderPass();
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
	m_swapchainPass.init();

	m_renderFramebufferProps.colorAttachmentCount = 1;
	m_renderFramebufferProps.useDepthAttachment = true;
	m_renderFramebufferProps.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	m_renderFramebufferProps.depthFormat = VK_FORMAT_D32_SFLOAT;

	m_renderPass.init(m_renderFramebufferProps);
	m_renderFramebuffer.init(m_renderFramebufferProps, m_renderPass, 1280, 720);

	m_shadowFramebufferProps.colorAttachmentCount = 0;
	m_shadowFramebufferProps.useDepthAttachment = true;
	m_shadowFramebufferProps.colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	m_shadowFramebufferProps.depthFormat = VK_FORMAT_D32_SFLOAT;

	m_shadowPass.init(m_shadowFramebufferProps);
	m_shadowFramebuffer.init(m_shadowFramebufferProps, m_shadowPass, 2048, 2048);
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	m_swapchain.init(m_renderFramebufferProps, m_swapchainPass, [&](uint32_t width, uint32_t height)
	{
			m_renderFramebuffer.resize(width, height);
	});
}

void Renderer::createGraphicsPipeline()
{
	{
		auto pipelineInfo = PipelineProps{};
		pipelineInfo.vertexPath = "resources/shaders/shadow/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/shadow/shader.frag.spv";
		pipelineInfo.descriptorSetLayouts = { m_descriptorPool.getLayout(0) };
		pipelineInfo.vertexInput = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		m_shadowPipeline.init(pipelineInfo, m_shadowFramebufferProps, m_shadowPass);
	}
	{
		auto pipelineInfo = PipelineProps{};
		pipelineInfo.vertexPath = "resources/shaders/main/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/main/shader.frag.spv";
		pipelineInfo.descriptorSetLayouts = { m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(1), m_descriptorPool.getLayout(1), m_descriptorPool.getLayout(1), m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(1) };
		pipelineInfo.vertexInput = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		m_renderPipeline.init(pipelineInfo, m_renderFramebufferProps, m_renderPass);
	}
	{
		auto pipelineInfo = PipelineProps{};
		pipelineInfo.vertexPath = "resources/shaders/skybox/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/skybox/shader.frag.spv";
		pipelineInfo.descriptorSetLayouts = { m_descriptorPool.getLayout(0), m_descriptorPool.getLayout(1) };
		pipelineInfo.vertexInput = true;
		pipelineInfo.depthWrite = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		m_skyboxPipeline.init(pipelineInfo, m_renderFramebufferProps, m_renderPass);
	}
	{
		auto pipelineInfo = PipelineProps{};
		pipelineInfo.vertexPath = "resources/shaders/combine/shader.vert.spv";
		pipelineInfo.fragmentPath = "resources/shaders/combine/shader.frag.spv";
		pipelineInfo.descriptorSetLayouts = { m_descriptorPool.getLayout(1), m_descriptorPool.getLayout(0) };
		pipelineInfo.vertexInput = false;
		pipelineInfo.culling = VK_CULL_MODE_NONE;
		m_combinePipeline.init(pipelineInfo, m_renderFramebufferProps, m_swapchainPass);
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

auto View =
#if 1
glm::lookAt(
	glm::vec3{ -4.0f, 6.0f, -4.0f },
	glm::vec3{ 0.0f, 0.5f, 0.0f },
	glm::vec3{ 0.0f, 1.0f, 0.0f }
);
#endif

auto Proj = 
#if 1 
glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -2.0f, 8.0f);
#else
glm::perspective(glm::radians(60.0f), 1.0f, 0.5f, 30.f);
#endif

void Renderer::renderShadows(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline)
{
	renderPass.begin(commandBuffer, m_shadowFramebuffer);
	setViewport(commandBuffer, 2048, 2048);
	pipeline.bind(commandBuffer);
	
	auto mvp = MVP{};
	mvp.proj = Proj;
	mvp.proj[1][1] *= -1.0f;
	mvp.view =
		glm::lookAt(
			-light.direction * 2.0f,
			glm::vec3{ 0.0f, 0.0f, 0.0f },
			glm::vec3{ 0.0f, 1.0f, 0.0f }
		);

	mvp.model = m_object.getModelMatrix();
	m_shadowMvp.write(mvp);
	m_shadowMvp.bind(commandBuffer, pipeline.getLayout(), 0);
	m_object.bindMesh(commandBuffer);
	m_object.draw(commandBuffer, pipeline.getLayout());

	mvp.model = m_plane.getModelMatrix();
	m_shadowMvp2.write(mvp);
	m_shadowMvp2.bind(commandBuffer, pipeline.getLayout(), 0);
	m_plane.bindMesh(commandBuffer);
	m_plane.draw(commandBuffer, pipeline.getLayout());

	renderPass.end(commandBuffer);
}

void Renderer::renderScene(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline)
{
	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration<float, std::chrono::seconds::period>(now - lastTime).count();
	lastTime = now;

	renderPass.begin(commandBuffer, m_renderFramebuffer);
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
	auto extent = m_swapchain.getExtent();

	auto view = m_camera.getViewMatrix();
	auto proj = glm::perspective(glm::radians(80.0f), extent.width / (float)extent.height, 0.1f, 100.0f);
	proj[1][1] *= -1;

	light.viewPosition = m_camera.getPosition();

	{
		m_skyboxPipeline.bind(commandBuffer);

		auto mvp = MVP{};
		mvp.view = glm::mat4{ glm::mat3{ view } };
		mvp.proj = proj;
		m_skyboxMvp.write(mvp);
		m_skyboxMvp.bind(commandBuffer, m_skyboxPipeline.getLayout(), 0);
		m_skybox.bind(commandBuffer, m_skyboxPipeline.getLayout(), 1);
		m_skyboxCube.bindMesh(commandBuffer);
		m_skyboxCube.draw(commandBuffer, m_skyboxPipeline.getLayout());
	}

	pipeline.bind(commandBuffer);
	m_skybox.bind(commandBuffer, pipeline.getLayout(), 7);
	{
		auto view =
			glm::lookAt(
				glm::normalize(-light.direction) * 2.0f,
				glm::vec3{ 0.0f, 0.0f, 0.0f },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			);
		auto proj = Proj;
		proj[1][1] *= -1;
		alignas (16) glm::mat4 lightSpace = proj * view;
		m_lightSpace.write(lightSpace);
		m_lightSpace.bind(commandBuffer, pipeline.getLayout(), 6);
	}

	m_light.write(light);
	m_light.bind(commandBuffer, pipeline.getLayout(), 1);
	m_shadowFramebuffer.getDepthTexture().bind(commandBuffer, pipeline.getLayout(), 5);

	m_specularMap.bind(commandBuffer, pipeline.getLayout(), 4);
	m_object.bindMVP(commandBuffer, pipeline.getLayout(), view, proj);
	m_object.bindMaterial(commandBuffer, pipeline.getLayout(), 2);
	m_object.bindTexture(commandBuffer, pipeline.getLayout(), 3);
	m_object.bindMesh(commandBuffer);
	m_object.draw(commandBuffer, pipeline.getLayout());

	m_planeSpecularMap.bind(commandBuffer, pipeline.getLayout(), 4);
	m_plane.bindMVP(commandBuffer, pipeline.getLayout(), view, proj);
	m_plane.bindMaterial(commandBuffer, pipeline.getLayout(), 2);
	m_plane.bindTexture(commandBuffer, pipeline.getLayout(), 3);
	m_plane.bindMesh(commandBuffer);
	m_plane.draw(commandBuffer, pipeline.getLayout());

	renderPass.end(commandBuffer);
}

void Renderer::combine(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline, uint32_t imageIndex)
{
	renderPass.begin(commandBuffer, m_swapchain.getFramebuffer(imageIndex));
	setViewport(commandBuffer);
	pipeline.bind(commandBuffer);
	{
		m_globalBuffer.write(m_global);
		m_globalBuffer.bind(commandBuffer, pipeline.getLayout(), 1);
	}
	m_renderFramebuffer.getColorTexture(0).bind(commandBuffer, pipeline.getLayout(), 0);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	renderPass.end(commandBuffer);
}

void Renderer::render()
{
	ZoneScopedN("render");

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	auto pos = m_object.getPosition();
	auto cachePos = m_object.getPosition();

	{
		ZoneScopedN("imgui");
		ImGui::Begin("Object");
		ImGui::DragFloat3("position", &pos.x, 0.1f);
		ImGui::Separator();
		ImGui::DragFloat("shininess", &m_object.material.shininess, 0.5f, 0.5f, 128.0f);
		ImGui::End();

		if (pos != cachePos) m_object.setPosition(pos);

		ImGui::Begin("Light");
		ImGui::DragFloat3("direction", (float*)&light.direction, 0.05f, -1.f, 1.f);
		ImGui::ColorEdit3("ambient", (float*)&light.ambient);
		ImGui::ColorEdit3("diffuse", (float*)&light.diffuse);
		ImGui::ColorEdit3("specular", (float*)&light.specular);
		ImGui::End();

		ImGui::Begin("Render");
		ImGui::DragFloat("gamma", (float*)&m_global.gamma, 0.05f, 0.f, 10.f);
		ImGui::End();

		ImGui::Render();
	}
	
	uint32_t imageIndex;
	{
		ZoneScopedN("acquire image");
		imageIndex = m_swapchain.beginFrame(m_inFlightFence, m_imageAvailableSemaphore);
		if (imageIndex == UINT32_MAX) return;
	}

	auto commandBuffer = m_commandBuffer;
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	{
		ZoneScopedN("shadow pass");
		renderShadows(commandBuffer, m_shadowPass, m_shadowPipeline);
	}
	{
		ZoneScopedN("main pass");
		renderScene(commandBuffer, m_renderPass, m_renderPipeline);
	}
	{
		ZoneScopedN("postproc pass");
		combine(commandBuffer, m_swapchainPass, m_combinePipeline, imageIndex);
	}

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };

	{
		ZoneScopedN("queue submit");
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
	}
	{
		ZoneScopedN("present");
		m_swapchain.endFrame(imageIndex, m_renderFinishedSemaphore);
	}
}