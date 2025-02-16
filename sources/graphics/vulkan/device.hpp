#pragma once

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context.hpp"
#include "graphics/vulkan/descriptor_pool.hpp"
#include "graphics/vulkan/buffer.hpp"

#include <memory>

class Device
{
public:
	Device(Context& context, VkSurfaceKHR surface);
	~Device();
	
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer);
	void copyBufferToImage(Buffer& srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, VkCommandBuffer commandBuffer = VK_NULL_HANDLE);

	std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count);
	DescriptorSet createDescriptorSet(VkDescriptorSetLayout layout);

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
	VkDescriptorPool getDescriptorPool();

	VkDescriptorSetLayout getUboVertexLayout();
	VkDescriptorSetLayout getUboFragmentLayout();
	//VkDescriptorSetLayout getSamplerVertexLayout();
	VkDescriptorSetLayout getSamplerFragmentLayout();

private:
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);

private:
	void pickGpu();
	bool isGpuSuitable(VkPhysicalDevice gpu);
	void createDevice();
	void createCommandPool();
	void createDescriptorSetLayouts();

private:
	Context& m_context;
	std::unique_ptr<DescriptorPool> m_descriptorPool;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkCommandPool m_commandPool;

	VkDescriptorSetLayout m_uboVertexLayout;
	VkDescriptorSetLayout m_uboFragmentLayout;
	//VkDescriptorSetLayout m_samplerVertexLayout;
	VkDescriptorSetLayout m_samplerFragmentLayout;
};