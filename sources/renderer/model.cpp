#include "renderer/model.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

Model::Model(Device& device, const std::string& modelPath, const std::string& texturePath)
: m_device{ device }, m_mesh{ new Mesh{device, modelPath} }, m_texture{ new Texture{device, texturePath} }
{}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t currentFrame)
{
	m_texture->bind(commandBuffer, layout, 1, currentFrame);
	m_mesh->bindBuffers(commandBuffer);
	m_mesh->draw(commandBuffer);
}