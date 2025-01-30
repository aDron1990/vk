#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

class Renderer
{
public:
	Renderer();
	~Renderer();
	void draw();

private:
	void createInstance();

	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	std::vector<const char*> getReqiuredExtensions();

	void pickGpu();
	bool isGpuSuitable(VkPhysicalDevice gpu);

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics;
	};
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

private:
#ifdef _DEBUG
	const bool USE_VALIDATION_LAYERS = true;
#else
	const bool USE_VALIDATION_LAYERS = false;
#endif
	const std::vector<const char*> VALIDATION_LAYER_NAMES = 
	{
		"VK_LAYER_KHRONOS_validation"
	};


	VkInstance m_instance{};
	VkDebugUtilsMessengerEXT m_debugMessenger{};
	VkPhysicalDevice m_gpu{};

};

