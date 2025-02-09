#pragma once

#include "renderer/config.hpp"
#include "renderer/types.hpp"
#include "renderer/context.hpp"
#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/graphics_pipeline.hpp"
#include "renderer/buffer.hpp"
#include "renderer/uniform_buffer.hpp"
#include "renderer/texture.hpp"
#include "renderer/mesh.hpp"
#include "renderer/model.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <memory>

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();
	void draw();

private:
	void createContext();
	void createDevice();
	void createSyncObjects();
	void createCommandBuffers();
	void createRenderPass();
	void createTexture();
	void createSwapchain();
	void createGraphicsPipeline();
	void createMesh();
	
private:
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
	uint32_t currentFrame{};
	GLFWwindow* m_window;

	VkRenderPass m_renderPass;

	struct FrameData
	{
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
	};
	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frameDatas;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Device> m_device;
	std::unique_ptr<Swapchain> m_swapchain;
	std::unique_ptr<GraphicsPipeline> m_pipeline;
	std::shared_ptr<Texture> m_texture;
	std::shared_ptr<Mesh> m_mesh;
	std::unique_ptr<Model> m_model;
};