#pragma once

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/swapchain.hpp"
#include "graphics/vulkan/pipeline.hpp"
#include "graphics/vulkan/buffer.hpp"
#include "graphics/vulkan/uniform_buffer.hpp"
#include "graphics/vulkan/texture.hpp"
#include "graphics/vulkan/mesh.hpp"
#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/camera.hpp"
#include "graphics/vulkan/object.hpp"
#include "graphics/vulkan/swapchain_pass.hpp"
#include "graphics/vulkan/offscreen_pass.hpp"

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

private:
	void setViewport(VkCommandBuffer commandBuffer);

private:
	Camera m_camera;
	Window& m_window;

	VkCommandBuffer m_commandBuffer;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Device> m_device;
	std::unique_ptr<SwapchainPass> m_renderPass;
	std::unique_ptr<Swapchain> m_swapchain;
	std::unique_ptr<Pipeline> m_pipeline;
	std::unique_ptr<LightBuffer> m_light;
	std::unique_ptr<Model> m_model;
	std::unique_ptr<Object> m_object;
	std::unique_ptr<Texture> m_specularMap;

	Light light{};
};