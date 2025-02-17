#pragma once

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context/context.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/context/swapchain.hpp"
#include "graphics/vulkan/pipeline.hpp"
#include "graphics/vulkan/buffer.hpp"
#include "graphics/vulkan/uniform_buffer.hpp"
#include "graphics/vulkan/texture.hpp"
#include "graphics/vulkan/mesh.hpp"
#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/camera.hpp"
#include "graphics/vulkan/object.hpp"
#include "graphics/vulkan/render_pass/swapchain_pass.hpp"
#include "graphics/vulkan/render_pass/offscreen_pass.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <memory>

using LightBuffer = UniformBuffer<Light>;

class Window;
class Renderer
{
public:
	Renderer(Window& window);
	~Renderer();
	void render();

private:
	void createContext();
	void createDevice();
	void createDescriptorPool();
	void createSyncObjects();
	void createCommandBuffers();
	void createRenderPass();
	void createSwapchain();
	void createGraphicsPipeline();
	
private:
	enum class Blur
	{
		Horizontal,
		Vertical
	};

	void renderShadows(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline);
	void renderScene(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline);
	void combine(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline, uint32_t imageIndex);

private:
	void setViewport(VkCommandBuffer commandBuffer);
	void setViewport(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);

private:
	Camera m_camera;
	Window& m_window;

	VkCommandBuffer m_commandBuffer;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	Context m_context;
	Device m_device;
	Swapchain m_swapchain;
	DescriptorPool m_descriptorPool;
	SwapchainPass m_swapchainPass;
	OffscreenPass m_renderPass;
	OffscreenPass m_shadowPass;
	Framebuffer m_renderFramebuffer;
	Framebuffer m_shadowFramebuffer;
	Pipeline m_combinePipeline;
	Pipeline m_renderPipeline;
	Pipeline m_shadowPipeline;
	LightBuffer m_light;
	Model m_model;
	Model m_planeModel;
	Object m_object;
	Object m_plane;
	Texture m_specularMap;
	Texture m_planeSpecularMap;

	UniformBuffer<MVP> m_shadowMvp;
	UniformBuffer<MVP> m_shadowMvp2;
	UniformBuffer<glm::mat4> m_lightSpace;
	Light light{};
	FramebufferProps m_renderFramebufferProps{};
	FramebufferProps m_shadowFramebufferProps{};
};