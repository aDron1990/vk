#pragma once

#include "renderer/config.hpp"
#include "renderer/types.hpp"
#include "renderer/context.hpp"
#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/buffer.hpp"
#include "renderer/uniform_buffer.hpp"
#include "renderer/texture.hpp"
#include "renderer/mesh.hpp"
#include "renderer/model.hpp"
#include "renderer/camera.hpp"
#include "renderer/object.hpp"
#include "renderer/swapchain_pass.hpp"
#include "renderer/offscreen_pass.hpp"

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
	void renderScene(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void postProccess(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
	Camera m_camera;
	Window& m_window;

	VkCommandBuffer m_commandBuffer;
	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	std::unique_ptr<Texture> test;
	std::unique_ptr<Texture> test2;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Device> m_device;
	std::unique_ptr<SwapchainPass> m_swapchainPass;
	std::unique_ptr<OffscreenPass> m_offscreenPass;
	std::unique_ptr<Swapchain> m_swapchain;
	std::unique_ptr<Pipeline> m_pipeline;
	std::unique_ptr<Pipeline> m_postPipeline;
	std::unique_ptr<LightBuffer> m_light;
	std::unique_ptr<Model> m_model;
	std::unique_ptr<Object> m_object;
	std::unique_ptr<Texture> m_specularMap;

	Light light{};
};