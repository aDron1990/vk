#pragma once

#include "renderer/swapchain.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

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
	void createInstance();

	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	std::vector<const char*> getReqiuredExtensions();
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void pickGpu();
	bool isGpuSuitable(VkPhysicalDevice gpu);
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);

	void createDevice();

	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void createCommandPool();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

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

	VkInstance m_instance{};
	VkDebugUtilsMessengerEXT m_debugMessenger{};
	VkPhysicalDevice m_gpu{};
	VkDevice m_device{};
	VkQueue m_graphicsQueue{};
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VkCommandPool m_commandPool{};

	std::unique_ptr<Swapchain> m_swapchain;
};

