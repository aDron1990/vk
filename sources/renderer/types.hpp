#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

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
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindDesc();
	static std::array<VkVertexInputAttributeDescription, 3> getAttrDesc();
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};