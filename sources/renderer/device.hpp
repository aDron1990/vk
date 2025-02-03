#pragma once

#include "renderer/types.hpp"
#include "renderer/context.hpp"

class Device
{
public:
	Device(Context& context, VkSurfaceKHR surface);
	~Device();
	
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	std::vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
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
	Context& m_context;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkCommandPool m_commandPool;
};