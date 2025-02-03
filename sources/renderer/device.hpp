#pragma once

#include "renderer/types.hpp"
#include "renderer/context.hpp"

class Device
{
public:
	Device(Context& context, VkSurfaceKHR surface);
	~Device();
	bool checkGpuExtensionsSupport(VkPhysicalDevice gpu);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpu);
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice gpu);
	VkSurfaceKHR getSurface();
	VkPhysicalDevice getGpu();
	VkDevice getDevice();
	VkQueue getGraphicsQueue();
	VkQueue getPresentQueue();

private:
	void pickGpu();
	bool isGpuSuitable(VkPhysicalDevice gpu);
	void createDevice();

private:
	Context& m_context;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_gpu;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
};