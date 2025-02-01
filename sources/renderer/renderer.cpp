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
	pickGpu();
	createDevice();
	createCommandPool();
	
	m_swapchain.reset(new Swapchain
	{{
		m_instance, m_gpu, m_device, m_commandPool, m_window,
		[this](VkPhysicalDevice gpu)
		{
			return findQueueFamilies(gpu);
		}
	}});
	createGraphicsPipeline();
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device);
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	m_swapchain.reset();
	vkDestroyDevice(m_device, nullptr);
	if (USE_VALIDATION_LAYERS) destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
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
			//auto swapchainSupport = querySwapchainSupport(gpu);
			swapchainAdequate = true;//!swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
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

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice gpu)
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
			indices.present = i;
			break;
		}
		i++;
	}

	/*i = int{};
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
	}*/

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
}

void Renderer::createGraphicsPipeline()
{
	auto vertexCode = loadFile("../../resources/shaders/shader.vert.spv");
	auto fragmentCode = loadFile("../../resources/shaders/shader.frag.spv");
	auto vertexModule = createShaderModule(vertexCode);
	auto fragmentModule = createShaderModule(fragmentCode);

	auto vertexStageInfo = VkPipelineShaderStageCreateInfo{};
	vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStageInfo.module = vertexModule;
	vertexStageInfo.pName = "main";

	auto fragmentStageInfo = VkPipelineShaderStageCreateInfo{};
	fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageInfo.module = fragmentModule;
	fragmentStageInfo.pName = "main";

	auto shaderStages = { vertexStageInfo, fragmentStageInfo };

	auto dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	auto dynamicInfo = VkPipelineDynamicStateCreateInfo{};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = dynamicStates.begin();
	dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

	auto viewportState = VkPipelineViewportStateCreateInfo{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	auto multisampling = VkPipelineMultisampleStateCreateInfo{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	auto blendingAttachment = VkPipelineColorBlendAttachmentState{};
	blendingAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
								VK_COLOR_COMPONENT_G_BIT |
								VK_COLOR_COMPONENT_B_BIT | 
								VK_COLOR_COMPONENT_A_BIT;
	blendingAttachment.blendEnable = VK_FALSE;

	auto blending = VkPipelineColorBlendStateCreateInfo{};
	blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blending.logicOpEnable = VK_FALSE;
	blending.attachmentCount = 1;
	blending.pAttachments = &blendingAttachment;

	auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan pipeline layout" };

	auto createInfo = VkGraphicsPipelineCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pStages = shaderStages.begin();
	createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pMultisampleState = &multisampling;
	createInfo.pDepthStencilState = nullptr;
	createInfo.pColorBlendState = &blending;
	createInfo.pDynamicState = &dynamicInfo;

	createInfo.layout = m_pipelineLayout;
	createInfo.renderPass = m_swapchain->getRenderPass();
	createInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_graphicsPipeline))
		throw std::runtime_error{ "failed to create vulkan pipeline" };

	vkDestroyShaderModule(m_device, vertexModule, nullptr);
	vkDestroyShaderModule(m_device, fragmentModule, nullptr);
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
	auto createInfo = VkShaderModuleCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.codeSize = code.size();

	auto shaderModule = VkShaderModule{};
	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create shader module" };
	
	return shaderModule;
}

void Renderer::createCommandPool()
{
	auto queueFamilyIndices = findQueueFamilies(m_gpu);

	auto createInfo = VkCommandPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphics.value();

	if (vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan command pool" };
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_swapchain->getRenderPass();
	renderPassInfo.framebuffer = m_swapchain->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();
	renderPassInfo.pClearValues = &clearColor;
	renderPassInfo.clearValueCount = 1;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain->getExtent().width);
	viewport.height = static_cast<float>(m_swapchain->getExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain->getExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };
}

void Renderer::draw()
{
	auto imageIndex = m_swapchain->beginFrame();
	auto commandBuffer = m_swapchain->getCommandBuffer();

	vkResetCommandBuffer(commandBuffer, 0);
	recordCommandBuffer(commandBuffer, imageIndex);

	std::initializer_list<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	auto submitInfo = VkSubmitInfo{};
	auto signalSemaphores = { m_swapchain->getRenderFinishedSemaphore() };
	auto waitSemaphores = { m_swapchain->getImageAvailableSemaphore()};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitSemaphores = waitSemaphores.begin();
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitDstStageMask = waitStages.begin();
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.begin();
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_swapchain->getInFlightFence()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to submit draw command buffer" };

	m_swapchain->endFrame(imageIndex);
}