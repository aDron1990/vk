#pragma once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/buffer.hpp"

#include <memory>
#include <string>
#include <vector>

class Mesh
{
public:
	void init(const std::string& modelPath);
	void bindBuffers(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

private:
	void loadModel(const std::string& modelPath);
	void createVertexBuffer();
	void createIndexBuffer();

private:
	bool m_initialized = false;
	Device* m_device{};
	std::vector<Vertex> m_vertices{};
	std::vector<uint32_t> m_indices{};
	std::unique_ptr<Buffer> m_vertexBuffer{};
	std::unique_ptr<Buffer> m_indexBuffer{};

};