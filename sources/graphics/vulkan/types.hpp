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
		return pos == other.pos 
			&& color == other.color 
			&& texCoord == other.texCoord 
			&& normal == other.normal;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			auto posHash = hash<glm::vec3>()(vertex.pos);
			auto colorHash = hash<glm::vec3>()(vertex.color);
			auto coordHash = hash<glm::vec2>()(vertex.texCoord);
			auto normalHash = hash<glm::vec2>()(vertex.normal);
			return posHash ^ (colorHash << 1) ^ (coordHash << 2) ^ (normalHash << 3);
		}
	};
}

struct ViewProjection
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct DirLight
{
	alignas(16) glm::vec3 direction {};
	alignas(16) glm::vec3 ambient { 0.2f };
	alignas(16) glm::vec3 diffuse { 0.5f };
	alignas(16) glm::vec3 specular{ 1.0f };
};

struct Material
{
	alignas(16) glm::vec3 diffuse{};
	alignas(16) glm::vec3 specular{};
	float shininess = 32.0f;
	int diffuseIndex = -1;
	int specularIndex = -1;
};
