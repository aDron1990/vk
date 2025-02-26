#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/locator.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

void Model::init(const std::string& modelPath)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_mesh.reset(new Mesh);
	m_mesh->init(modelPath);
}

void Model::bindMesh(VkCommandBuffer commandBuffer)
{
	assert(m_initialized);
	m_mesh->bindBuffers(commandBuffer);
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout)
{
	assert(m_initialized);
	m_mesh->draw(commandBuffer);
}