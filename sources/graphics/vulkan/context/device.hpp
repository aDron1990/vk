#pragma once

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context/context.hpp"
#include "graphics/vulkan/descriptor/descriptor_pool.hpp"
#include "graphics/vulkan/buffer.hpp"

#include <memory>

class Device
{
public:
	~Device();
	void init(VkSurfaceKHR surface);
	void destroy();
	
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkImageView createImageView(VkImage image, uint32_t layerCount, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer);
	void copyBufferToImage(Buffer& srcBuffer, VkImage dstImage, uint32_t width, uint32_t height, uint32_t layerCount = 1);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);
	void transitionImageLayout(VkImage image, uint32_t layerCount, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

	std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count);
	DescriptorSetPtr createDescriptorSet(VkDescriptorSetLayout layout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);

	VkSurfaceKHR getSurface();
	VkPhysicalDevice getGpu();
	VkDevice getDevice();
	VkQueue getGraphicsQueue();
	VkQueue getPresentQueue();

private:
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);

private:
	void pickGpu();
	bool isGpuSuitable(VkPhysicalDevice gpu);
	void createDevice();
	void createCommandPool();

private:
	bool m_initialized = false;
	Context* m_context{};
	VkSurfaceKHR m_surface{};
	VkPhysicalDevice m_gpu{};
	VkDevice m_device{};
	VkQueue m_graphicsQueue{};
	VkQueue m_presentQueue{};
	VkCommandPool m_commandPool{};
};