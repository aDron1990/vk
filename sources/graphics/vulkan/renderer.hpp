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
	void renderScene(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline);
	void combine(VkCommandBuffer commandBuffer, RenderPass& renderPass, Pipeline& pipeline, uint32_t imageIndex);

private:
	void setViewport(VkCommandBuffer commandBuffer);

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
	OffscreenPass m_testPass;
	Framebuffer m_testFramebuffer;
	Pipeline m_combinePipeline;
	Pipeline m_testPipeline;
	LightBuffer m_light;
	Model m_model;
	Object m_object;
	Texture m_specularMap;

	Light light{};
	FramebufferProps m_framebufferProps{};
};