#pragma once

#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

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
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindDesc();
	static std::array<VkVertexInputAttributeDescription, 4> getAttrDesc();
	inline bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct MVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Light
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 viewPosition;
	alignas(16) glm::vec3 color;
};