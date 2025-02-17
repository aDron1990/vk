#pragma once

#include "graphics/vulkan/config.hpp"

#include <vulkan/vulkan.h>

#include <vector>

class Context
{
public:
	~Context();
	void init();
	void destroy();

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
	bool m_initialized = false;
	VkInstance m_instance{};
	VkDebugUtilsMessengerEXT m_debugMessenger{};
};