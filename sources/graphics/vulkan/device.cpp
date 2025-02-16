#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/locator.hpp"

#include <print>
#include <set>
#include <string>
#include <stdexcept>

Device::Device(Context& context, VkSurfaceKHR surface, const DescriptorPoolProps& poolProps) : m_context{context}, m_surface{surface}
{
	pickGpu();
	createDevice();
	createCommandPool();
	m_descriptorPool.reset(new DescriptorPool{ *this, poolProps });
	Locator::setDevice(this);
}

Device::~Device()
{
	m_descriptorPool.reset();
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_context.getInstance(), m_surface, nullptr);
}

void Device::pickGpu()
{
	auto gpuCount = uint32_t{};
	vkEnumeratePhysicalDevices(m_context.getInstance(), &gpuCount, nullptr);
	auto gpus = std::vector<VkPhysicalDevice>(gpuCount);
	vkEnumeratePhysicalDevices(m_context.getInstance(), &gpuCount, gpus.data());
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

bool Device::isGpuSuitable(VkPhysicalDevice gpu)
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
		return true && indices.graphics.has_value() && indices.present.has_value() && extensionsSupport && swapchainAdequate && gpuFeatures.samplerAnisotropy;
	}

	return false;
}

bool Device::checkGpuExtensionsSupport(VkPhysicalDevice gpu)
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

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	auto memProps = VkPhysicalDeviceMemoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_gpu, &memProps);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	throw std::runtime_error{ "failed to find suitable memory type!" };
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice gpu)
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

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (auto format : candidates)
	{
		auto props = VkFormatProperties{};
		vkGetPhysicalDeviceFormatProperties(m_gpu, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat Device::findDepthFormat()
{
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

SwapchainSupportDetails Device::querySwapchainSupport(VkPhysicalDevice gpu)
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

void Device::createDevice()
{
	auto indices = findQueueFamilies(m_gpu);
	auto uniqueQueueFamilies = std::set<uint32_t>{ indices.graphics.value(), indices.present.value() };
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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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

void Device::createCommandPool()
{
	auto queueFamilyIndices = findQueueFamilies(m_gpu);
	auto createInfo = VkCommandPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphics.value();
	if (vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan command pool" };
}

void Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	auto createInfo = VkBufferCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vertex buffer" };

	auto memReq = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(m_device, buffer, &memReq);

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vertex buffer memory" };

	vkBindBufferMemory(m_device, buffer, memory, 0);
}

void Device::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	auto createInfo = VkImageCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(m_device, &createInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create image" };

	auto memReq = VkMemoryRequirements{};
	vkGetImageMemoryRequirements(m_device, image, &memReq);

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate image memory" };

	vkBindImageMemory(m_device, image, imageMemory, 0);
}

VkImageView Device::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	auto imageView = VkImageView{};
	auto createInfo = VkImageViewCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(m_device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create image view" };

	return imageView;
}

void Device::copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer)
{
	auto commandBuffer = beginSingleTimeCommands();
	
	auto copyRegion = VkBufferCopy{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = srcBuffer.getSize();
	vkCmdCopyBuffer(commandBuffer, srcBuffer.getBuffer(), dstBuffer.getBuffer(), 1, &copyRegion); 

	endSingleTimeCommands(commandBuffer);
}

void Device::copyBufferToImage(Buffer& srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	auto commandBuffer = beginSingleTimeCommands();

	auto region = VkBufferImageCopy{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};
	vkCmdCopyBufferToImage(commandBuffer, srcBuffer.getBuffer(), dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

std::vector<VkCommandBuffer> Device::createCommandBuffers(uint32_t count)
{
	auto commandBuffers = std::vector<VkCommandBuffer>(count);
	auto allocInfo = VkCommandBufferAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	if (vkAllocateCommandBuffers(m_device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vulkan command buffers" };

	return commandBuffers;
}

DescriptorSet Device::createDescriptorSet(VkDescriptorSetLayout layout)
{
	return m_descriptorPool->createSet(layout);
}

VkCommandBuffer Device::beginSingleTimeCommands()
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Device::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkImageAspectFlags aspect, VkCommandBuffer commandBuffer)
{
	auto end = false;
	if (commandBuffer == VK_NULL_HANDLE)
	{
		commandBuffer = beginSingleTimeCommands();
		end = true;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	auto barrier = VkImageMemoryBarrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	if (end) endSingleTimeCommands(commandBuffer);
}

VkSurfaceKHR Device::getSurface()
{
	return m_surface;
}

VkPhysicalDevice Device::getGpu()
{
	return m_gpu;
}

VkDevice Device::getDevice()
{
	return m_device;
}

VkQueue Device::getGraphicsQueue()
{
	return m_graphicsQueue;
}

VkQueue Device::getPresentQueue()
{
	return m_presentQueue;
}

VkDescriptorPool Device::getDescriptorPool()
{
	return m_descriptorPool->getPool();
}

VkDescriptorSetLayout Device::getUboVertexLayout()
{
	return m_descriptorPool->getLayout(0);
}

VkDescriptorSetLayout Device::getUboFragmentLayout()
{
	return m_descriptorPool->getLayout(0);
}

VkDescriptorSetLayout Device::getSamplerFragmentLayout()
{
	return m_descriptorPool->getLayout(1);
}
