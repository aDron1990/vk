#include "renderer/renderer.hpp"
#include "load_file.hpp"

#include <stdexcept>
#include <print>
#include <set>
#include <algorithm>
#include <limits>
#include <cstdint>

Renderer::Renderer(GLFWwindow* window) : m_window{window}
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickGpu();
	createDevice();
	createSwapchain();
	createImageViews();
	createGraphicsPipeline();
}

Renderer::~Renderer()
{
	for (auto view : m_swapchainImageViews)
		vkDestroyImageView(m_device, view, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
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
	auto extensionsSupport = checkGpuExtensionsSupport(gpu);
	vkGetPhysicalDeviceProperties(gpu, &gpuProperties);
	vkGetPhysicalDeviceFeatures(gpu, &gpuFeatures);
	if (gpuProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		gpuFeatures.geometryShader)
	{
		std::println("GPU: {}", gpuProperties.deviceName);
		auto swapchainAdequate = false;
		if (extensionsSupport)
		{
			auto swapchainSupport = querySwapchainSupport(gpu);
			swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		}
		return true && indices.graphics.has_value() && indices.present.has_value() && extensionsSupport && swapchainAdequate;
	}
	return false;
}

bool Renderer::checkGpuExtensionsSupport(VkPhysicalDevice gpu)
{
	auto extensionCount = uint32_t{};
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);
	auto availableExtensions = std::vector<VkExtensionProperties>(extensionCount);
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

	auto requiredExtensions = std::set<std::string>(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
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
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
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

void Renderer::createSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan surface" };
}

Renderer::SwapchainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice gpu)
{
	auto details = SwapchainSupportDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, m_surface, &details.capabilities);

	auto formatCount = uint32_t{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, nullptr);
	if (formatCount != 0) 
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, m_surface, &formatCount, details.formats.data());
	}

	auto presentModeCount = uint32_t{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	for (const auto& mode : availableModes) 
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		auto width = int{};
		auto height = int{};
		glfwGetFramebufferSize(m_window, &width, &height);
		auto actualExtent = VkExtent2D
		{
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

void Renderer::createSwapchain()
{
	auto swapchainDetails = querySwapchainSupport(m_gpu);
	auto surfaceFormat = chooseSwapchainSurfaceFormat(swapchainDetails.formats);
	auto presentMode = chooseSwapchainPresentMode(swapchainDetails.presentModes);
	auto extent = chooseSwapchainExtent(swapchainDetails.capabilities);

	auto imageCount = swapchainDetails.capabilities.minImageCount + 1;
	if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount)
		imageCount = swapchainDetails.capabilities.maxImageCount;

	auto createInfo = VkSwapchainCreateInfoKHR{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = findQueueFamilies(m_gpu);
	if (indices.graphics != indices.present)
	{
		auto queueFamilyIndices = { indices.graphics.value(), indices.present.value() };
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = queueFamilyIndices.begin();
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	}
	else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE; //!!!

	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan swapchain" };

	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
	m_swapchainFormat = surfaceFormat.format;
	m_swapchainExtent = extent;
}

void Renderer::createImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());
	for (int i = 0; i < m_swapchainImages.size(); i++)
	{
		auto createInfo = VkImageViewCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]))
			throw std::runtime_error{ "failed to create swapchain image view" };
	}
}

void Renderer::createGraphicsPipeline()
{
	auto vertexCode = loadFile("../../resources/shaders/shader.vert.spv");
	auto fragmentCode = loadFile("../../resources/shaders/shader.frag.spv");
}