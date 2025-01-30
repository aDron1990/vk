#include "renderer/renderer.hpp"

#include <stdexcept>
#include <print>
#include <set>
#include <cstdint>

Renderer::Renderer(GLFWwindow* window)
{
	createInstance();
	setupDebugMessenger();
	createSurface(window);
	pickGpu();
	createDevice();
}

Renderer::~Renderer()
{
	vkDestroyDevice(m_device, nullptr);
	if (USE_VALIDATION_LAYERS) destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void Renderer::createInstance()
{
	auto appInfo = VkApplicationInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto requiredExtensions = getReqiuredExtensions();

	auto instanceInfo = VkInstanceCreateInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	if (USE_VALIDATION_LAYERS)
	{
		auto debugInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		populateDebugMessengerCreateInfo(debugInfo);
		instanceInfo.pNext = &debugInfo;
		instanceInfo.ppEnabledLayerNames = VALIDATION_LAYER_NAMES.data();
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYER_NAMES.size());
	}
	else
		instanceInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS || !checkValidationLayerSupport())
	{
		throw std::runtime_error{ "failed to create VkInstance" };
	}
}

void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
								VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char*> Renderer::getReqiuredExtensions()
{
	auto glfwExtensionCount = uint32_t{};
	const char** glfwExtensions{};
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	auto requiredExtensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (USE_VALIDATION_LAYERS)
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return requiredExtensions;
}

bool Renderer::checkValidationLayerSupport()
{
	auto availableLayerCount = uint32_t{};
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

	auto availableLayers = std::vector<VkLayerProperties>(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

 	for (const auto* layerName : VALIDATION_LAYER_NAMES)
	{
		auto layerFound = false;
		for (const auto& layer : availableLayers)
		{
			if (strcmp(layerName, layer.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) return false;
	}

	return true;
}

void Renderer::setupDebugMessenger()
{
	if (!USE_VALIDATION_LAYERS) return;
	auto createInfo = VkDebugUtilsMessengerCreateInfoEXT{};
	populateDebugMessengerCreateInfo(createInfo);
	if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::println("validation layer: {}", pCallbackData->pMessage);
	return VK_FALSE;
}

VkResult Renderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Renderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

void Renderer::pickGpu()
{
	auto gpuCount = uint32_t{};
	vkEnumeratePhysicalDevices(m_instance, &gpuCount, nullptr);
	auto gpus = std::vector<VkPhysicalDevice>(gpuCount);
	vkEnumeratePhysicalDevices(m_instance, &gpuCount, gpus.data());
	for (const auto& gpu : gpus)
	{
		if (isGpuSuitable(gpu))
		{
			m_gpu = gpu;
			break;
		}
	}

	if (m_gpu == VK_NULL_HANDLE)
		throw std::runtime_error{ "failed to pick gpu" };
}

bool Renderer::isGpuSuitable(VkPhysicalDevice gpu)
{
	auto indices = findQueueFamilies(gpu);
	auto gpuProperties = VkPhysicalDeviceProperties{};
	auto gpuFeatures = VkPhysicalDeviceFeatures{};
	vkGetPhysicalDeviceProperties(gpu, &gpuProperties);
	vkGetPhysicalDeviceFeatures(gpu, &gpuFeatures);
	if (gpuProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		gpuFeatures.geometryShader)
	{
		std::println("GPU: {}", gpuProperties.deviceName);
		return true && indices.graphics.has_value() && indices.present.has_value();
	}
	return false;
}

Renderer::QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice gpu)
{
	auto indices = QueueFamilyIndices{};
	
	auto queueFamilyCount = uint32_t{};
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
	auto queueFamilies = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

	auto i = int{};
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics = i;
			break;
		}
		i++;
	}

	i = int{};
	for (const auto& queueFamily : queueFamilies)
	{
		auto presentSupport = VkBool32{};
		vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_surface, &presentSupport);
		if (presentSupport)
		{
			indices.present = i;
			break;
		}
		i++;
	}

	return indices;
}

void Renderer::createDevice()
{
	auto indices = findQueueFamilies(m_gpu);
	auto uniqueQueueFamilies = std::set<uint32_t>{indices.graphics.value(), indices.present.value() };
	auto queueCreateInfos = std::vector<VkDeviceQueueCreateInfo>{};
	queueCreateInfos.reserve(2);

	for (auto& queueFamily : uniqueQueueFamilies)

	{
		auto queueCreateInfo = VkDeviceQueueCreateInfo{};
		auto queuePriority = 1.0f;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	auto deviceFeatures = VkPhysicalDeviceFeatures{};

	auto createInfo = VkDeviceCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;
	if (USE_VALIDATION_LAYERS)
	{
		createInfo.ppEnabledLayerNames = VALIDATION_LAYER_NAMES.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYER_NAMES.size());
	}
	else
		createInfo.enabledLayerCount = 0;
	if (vkCreateDevice(m_gpu, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan device" };

	vkGetDeviceQueue(m_device, indices.graphics.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.present.value(), 0, &m_presentQueue);
}

void Renderer::createSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan surface" };
}