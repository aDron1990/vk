#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/locator.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

Model::Model(Device& device, const std::string& modelPath, const std::string& texturePath)
: m_device{ device }, m_mesh{ new Mesh }, m_texture{ new Texture{} }
{
	m_mesh->init(modelPath);
	m_texture->init(texturePath, Locator::getDescriptorPool().createSet(1));
}

void Model::bindTexture(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t  set)
{
	m_texture->bind(commandBuffer, layout, set);
}

void Model::bindMesh(VkCommandBuffer commandBuffer)
{
	m_mesh->bindBuffers(commandBuffer);
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout)
{
	m_mesh->draw(commandBuffer);
}