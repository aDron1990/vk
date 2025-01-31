#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

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
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> present;
	};
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);

	void createDevice();

	void createSurface();
	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);
	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapchain();
	void createImageViews();

	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);

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

	GLFWwindow* m_window;

	VkInstance m_instance{};
	VkDebugUtilsMessengerEXT m_debugMessenger{};
	VkPhysicalDevice m_gpu{};
	VkDevice m_device{};
	VkQueue m_graphicsQueue{};
	VkQueue m_presentQueue{};
	VkSurfaceKHR m_surface{};
	VkSwapchainKHR m_swapchain{};
	std::vector<VkImageView> m_swapchainImageViews{};
	VkPipelineLayout m_pipelineLayout{};


	std::vector<VkImage> m_swapchainImages{};
	VkFormat m_swapchainFormat{};
	VkExtent2D m_swapchainExtent{};
};

