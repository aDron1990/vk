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

const std::string MODEL_PATH = "resources/models/torus.obj";
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

	m_model.init(MODEL_PATH);
	m_plane.init("resources/models/plane.obj");
	m_vpBuffer.init(m_descriptorPool.createSet(0));
	m_view.init(m_descriptorPool.createSet(0));
	m_dirLight.init(m_descriptorPool.createSet(0));
	dirLight.direction = { 0.0f, -1.0f, 0.0f };
	m_dirLight.write(dirLight);

	m_textures.init(128);
	m_textures.addTexture("resources/images/container2.png", "container_diffuse");
	m_textures.addTexture("resources/images/container2_specular.png", "container_specular");
	m_textures.addTexture("resources/images/statue.jpg", "statue");

	m_materialBuffer.init(64, m_descriptorPool.createSet(2));
	auto material = Material{};

	m_1.init(m_model, m_materialBuffer);
	material.diffuse = {1.0f, 0.0f, 0.0f};
	m_1.setMaterial(material);
	m_1.setPosition({ -1.5f, 1.0f, 0.0f });

	m_floor.init(m_plane, m_materialBuffer);
	material.diffuse = { 0.8f, 0.5f, 0.5f };
	m_floor.setMaterial(material);

	m_2.init(m_model, m_materialBuffer);
	material.diffuseIndex = m_textures.findIndex("container_diffuse");
	material.specularIndex = m_textures.findIndex("container_specular");
	m_2.setMaterial(material);
	m_2.setPosition({ 1.5f, 1.0f, 0.0f });

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
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
		}, VK_SHADER_STAGE_ALL_GRAPHICS, 100 },
		DescriptorSetInfo
		{{
			BindingInfo
			{
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			}
		}, VK_SHADER_STAGE_ALL_GRAPHICS, 1 },
		DescriptorSetInfo
		{{
			BindingInfo
			{
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 128
			}
		}, VK_SHADER_STAGE_ALL_GRAPHICS, 128 },
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
		pipelineInfo.vertexInput = true;
		pipelineInfo.usePushConstants = true;
		pipelineInfo.culling = VK_CULL_MODE_BACK_BIT;
		pipelineInfo.descriptorSetLayouts =
		{
			m_descriptorPool.getLayout(0),
			m_descriptorPool.getLayout(2),
			m_descriptorPool.getLayout(0),
			m_descriptorPool.getLayout(0),
			m_descriptorPool.getLayout(3),
		};
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
	auto vp = ViewProjection{};
	vp.view = m_camera.getViewMatrix();
	vp.proj = glm::perspective(glm::radians(80.0f), extent.width / (float)extent.height, 0.1f, 100.0f);
	vp.proj[1][1] *= -1;
	m_vpBuffer.write(vp);
	m_vpBuffer.bind(commandBuffer, pipeline.getLayout(), 0);
	m_view.write(m_camera.getPosition());
	m_view.bind(commandBuffer, pipeline.getLayout(), 2);
	m_dirLight.bind(commandBuffer, pipeline.getLayout(), 3);

	m_textures.bind(commandBuffer, pipeline.getLayout(), 4);
	glm::mat4 model;

	model = m_1.getModelMatrix();
	vkCmdPushConstants(commandBuffer, pipeline.getLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(model), &model);
	m_materialBuffer.bind(m_1.getMaterialIndex(), commandBuffer, pipeline.getLayout(), 1);
	m_1.draw(commandBuffer, pipeline.getLayout());

	model = m_2.getModelMatrix();
	vkCmdPushConstants(commandBuffer, pipeline.getLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(model), &model);
	m_materialBuffer.bind(m_2.getMaterialIndex(), commandBuffer, pipeline.getLayout(), 1);
	m_2.draw(commandBuffer, pipeline.getLayout());

	model = m_floor.getModelMatrix();
	vkCmdPushConstants(commandBuffer, pipeline.getLayout(), VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(model), &model);
	m_materialBuffer.bind(m_floor.getMaterialIndex(), commandBuffer, pipeline.getLayout(), 1);
	m_floor.draw(commandBuffer, pipeline.getLayout());

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	renderPass.end(commandBuffer);
}

void Renderer::render()
{
	ZoneScopedN("render");

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGui::Begin("Directional Light");
		bool updateLight = false;
		updateLight = updateLight || ImGui::DragFloat3("direction", (float*)&dirLight.direction, 0.05f, -1.0f, 1.0f);
		updateLight = updateLight || ImGui::ColorEdit3("ambient", (float*)&dirLight.ambient);
		updateLight = updateLight || ImGui::ColorEdit3("diffuse", (float*)&dirLight.diffuse);
		updateLight = updateLight || ImGui::ColorEdit3("specular", (float*)&dirLight.specular);
		if (updateLight)
		{
			dirLight.direction = glm::normalize(dirLight.direction);
			m_dirLight.write(dirLight);
		}
		ImGui::End();
	}
	{
		ImGui::Begin("Torus 1");
		ImGui::Text("Transform");
		auto pos = m_1.getPosition();
		if (ImGui::DragFloat3("position", (float*)&pos, 0.05f)) m_1.setPosition(pos);
		auto rot = m_1.getRotation();
		if (ImGui::DragFloat3("rotation", (float*)&rot)) m_1.setRotation(rot);
		auto scale = m_1.getScale ();
		if (ImGui::DragFloat3("scale", (float*)&scale)) m_1.setScale(scale);
		ImGui::Separator();

		ImGui::Text("Material");
		auto material = m_1.getMaterial();
		bool updateMaterial = false;
		updateMaterial = updateMaterial || ImGui::ColorEdit3("diffuse", (float*)&material.diffuse);
		updateMaterial = updateMaterial || ImGui::ColorEdit3("specular", (float*)&material.specular);
		updateMaterial = updateMaterial || ImGui::DragFloat("shininess", (float*)&material.shininess, 0.5f, 1.0f, 128.0f);
		if (updateMaterial) m_1.setMaterial(material);
		ImGui::End();
	}
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