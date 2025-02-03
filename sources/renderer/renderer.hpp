#pragma once

#include "renderer/config.hpp"
#include "renderer/context.hpp"
#include "renderer/swapchain.hpp"

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
	void createSurface();
	void pickGpu();
	void createDevice();
	void createCommandPool();
	void createSyncObjects();
	void createCommandBuffers();
	void createRenderPass();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createSwapchain();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();

private:
	bool isGpuSuitable(VkPhysicalDevice gpu);
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void updateUniformBuffer();

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame{};

	GLFWwindow* m_window;

	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkRenderPass m_renderPass;
	VkDescriptorSetLayout m_descriptorLayout;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_uniformBuffersMapped;

	std::unique_ptr<Context> m_context;
	std::unique_ptr<Swapchain> m_swapchain;
};

