#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <optional>
#include <array>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

using FindQueueFamilyFunc = std::function<QueueFamilyIndices(VkPhysicalDevice gpu)>;

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static constexpr VkVertexInputBindingDescription getBindDesc();
	static constexpr std::array<VkVertexInputAttributeDescription, 2> getAttrDesc();
};