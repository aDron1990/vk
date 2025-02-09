#pragma once

#include "renderer/types.hpp"
#include "renderer/device.hpp"
#include "renderer/buffer.hpp"

#include <memory>
#include <string>
#include <vector>

class Mesh
{
public:
	Mesh(Device& device, const std::string& modelPath);
	void bindBuffers(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

private:
	void loadModel(const std::string& modelPath);
	void createVertexBuffer();
	void createIndexBuffer();

private:
	Device& m_device;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	std::unique_ptr<Buffer> m_vertexBuffer;
	std::unique_ptr<Buffer> m_indexBuffer;

};