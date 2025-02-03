#include "renderer/types.hpp"

constexpr VkVertexInputBindingDescription Vertex::getBindDesc()
{
	auto bindDesc = VkVertexInputBindingDescription{};
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(Vertex);
	bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindDesc;
}

constexpr std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttrDesc()
{
	auto attrDesc = std::array<VkVertexInputAttributeDescription, 2>{};

	attrDesc[0].binding = 0;
	attrDesc[0].location = 0;
	attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc[0].offset = offsetof(Vertex, pos);

	attrDesc[1].binding = 0;
	attrDesc[1].location = 1;
	attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDesc[1].offset = offsetof(Vertex, color);

	return attrDesc;
}