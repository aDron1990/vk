#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class Context
{
public:
	Context();
	~Context();
	VkInstance getInstance();

private:
	void createInstance();
	bool checkValidationLayerSupport();
	void setupDebugMessenger();
	std::vector<const char*> getReqiuredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

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
	const std::vector<const char*> VALIDATION_LAYER_NAMES =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	const std::vector<const char*> DEVICE_EXTENSIONS =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
};