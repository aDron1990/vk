#pragma once

#include "renderer/config.hpp"
#include "renderer/context.hpp"
#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/buffer.hpp"
#include "renderer/uniform_buffer.hpp"

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
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorSetLayout();
	void createDescriptorSets();
	void createSwapchain();
	void createGraphicsPipeline();

private:
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer();

private:
	uint32_t currentFrame{};
	GLFWwindow* m_window;

	VkRenderPass m_renderPass;
	VkDescriptorSetLayout m_descriptorLayout;

	VkImage m_textureImage;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;
	VkDeviceMemory m_textureImageMemory;

	struct FrameData
	{
		VkDescriptorSet descriptorSet;
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
	};
	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_frameDatas;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Device> m_device;
	std::unique_ptr<UniformBuffer<UniformBufferObject>> m_uniformBuffer;
	std::unique_ptr<Swapchain> m_swapchain;
	std::unique_ptr<Pipeline> m_pipeline;
	std::unique_ptr<Buffer> m_vertexBuffer;
	std::unique_ptr<Buffer> m_indexBuffer;
};

