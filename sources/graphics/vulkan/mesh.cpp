#include "graphics/vulkan/mesh.hpp"
#include "graphics/vulkan/locator.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(models);

void Mesh::init(const std::string& modelPath)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	loadModel(modelPath);
	createVertexBuffer();
	createIndexBuffer();
}

void Mesh::loadModel(const std::string& modelPath)
{
	auto reader = tinyobj::ObjReader{};
	auto modelFile = cmrc::models::get_filesystem().open(modelPath);
	auto modelString = std::string{ modelFile.begin(), modelFile.size() };

	auto mtlPath = modelPath.substr(0, modelPath.find_last_of(".")) + ".mtl";
	auto mtlFile = cmrc::models::get_filesystem().open(mtlPath);
	auto mtlString = std::string{ mtlFile.begin(), mtlFile.size() };

	reader.ParseFromString(modelString, mtlString);
	auto attrib = reader.GetAttrib();
	auto shapes = reader.GetShapes();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 0.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}
			m_indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Mesh::createVertexBuffer()
{
	VkDeviceSize size = sizeof(m_vertices[0]) * m_vertices.size();
	Buffer stagingBuffer;
	stagingBuffer.init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	auto* data = stagingBuffer.map();
	memcpy(data, m_vertices.data(), static_cast<size_t>(size));
	stagingBuffer.unmap();
	m_vertexBuffer.reset(new Buffer);
	m_vertexBuffer->init(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_device->copyBuffer(stagingBuffer, *m_vertexBuffer);
}

void Mesh::createIndexBuffer()
{
	VkDeviceSize size = sizeof(m_indices[0]) * m_indices.size();
	Buffer stagingBuffer;
	stagingBuffer.init(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	auto* data = stagingBuffer.map();
	memcpy(data, m_indices.data(), static_cast<size_t>(size));
	stagingBuffer.unmap();
	m_indexBuffer.reset(new Buffer);
	m_indexBuffer->init(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	m_device->copyBuffer(stagingBuffer, *m_indexBuffer);
}

void Mesh::bindBuffers(VkCommandBuffer commandBuffer)
{
	assert(m_initialized);
	VkDeviceSize offsets[] = { 0 };
	auto buffer = m_vertexBuffer->getBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer commandBuffer)
{
	assert(m_initialized);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}