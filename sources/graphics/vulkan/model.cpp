#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/locator.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

void Model::init(const std::string& modelPath, const std::string& texturePath)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_mesh.reset(new Mesh);
	m_texture.reset(new Texture);
	m_mesh->init(modelPath);
	m_texture->init(texturePath, Locator::getDescriptorPool().createSet(1));
}

void Model::bindTexture(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t  set)
{
	assert(m_initialized);
	m_texture->bind(commandBuffer, layout, set);
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