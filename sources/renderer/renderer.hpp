#pragma once

#include "renderer/swapchain.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <memory>

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static constexpr VkVertexInputBindingDescription getBindDesc();
	static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttrDesc();
};

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();
	void draw();

private:
	void createInstance();
	void createSurface();
	void pickGpu();
	void createDevice();
	void createCommandPool();
	void createRenderPass();
	void createVertexBuffer();
	void createSwapchain();
	void createGraphicsPipeline();

private:
	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	std::vector<const char*> getReqiuredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	bool isGpuSuitable(VkPhysicalDevice gpu);
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

private:
#ifdef _DEBUG
	const bool USE_VALIDATION_LAYERS = true;
#else
	const bool USE_VALIDATION_LAYERS = false;
#endif

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> VALIDATION_LAYER_NAMES = 
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> DEVICE_EXTENSIONS = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	GLFWwindow* m_window;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	VkCommandPool m_commandPool;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	std::unique_ptr<Swapchain> m_swapchain;
};

