#pragma once

#include "renderer/config.hpp"
#include "renderer/context.hpp"
#include "renderer/device.hpp"
#include "renderer/swapchain.hpp"
#include "renderer/buffer.hpp"

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
	void createDescriptorPool();
	void createDescriptorSets();
	void createSwapchain();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();

private:
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer();
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame{};

	GLFWwindow* m_window;

	VkRenderPass m_renderPass;
	VkDescriptorSetLayout m_descriptorLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	VkImage m_textureImage;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;
	VkDeviceMemory m_textureImageMemory;
	std::unique_ptr<Buffer> m_vertexBuffer;
	//VkBuffer m_vertexBuffer;
	//VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_uniformBuffersMapped;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Device> m_device;
	std::unique_ptr<Swapchain> m_swapchain;
};

