#include "renderer/model.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

Model::Model(Device& device, MeshPtr mesh, TexturePtr texture)
	: m_device{ device }, m_mesh{ mesh }, m_texture{ texture }, m_mvpBuffer{device}
{

}

Model::~Model()
{
	
}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t currentFrame, const glm::mat4& view, const glm::mat4& proj)
{
	auto ubo = UniformBufferObject{};
	ubo.model = glm::translate(glm::mat4(1.0f), m_position);
	ubo.model = glm::rotate(ubo.model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo.model = glm::rotate(ubo.model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = glm::rotate(ubo.model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.model = glm::scale(ubo.model, m_scale);
	ubo.view = view;
	ubo.proj = proj;
	m_mvpBuffer.write(ubo, currentFrame);
	m_texture->bind(commandBuffer, layout, currentFrame);
	m_mvpBuffer.bind(commandBuffer, layout, currentFrame);
	m_mesh->bindBuffers(commandBuffer);
	m_mesh->draw(commandBuffer);
}

void Model::setPosition(glm::vec3 position)
{
	m_position = position;
}

void Model::setRotation(glm::vec3 rotation)
{
	m_rotation = rotation;
}

void Model::setScale(glm::vec3 scale)
{
	m_scale = scale;
}

glm::vec3 Model::getPosition()
{
	return m_position;
}

glm::vec3 Model::getRotation()
{
	return m_rotation;
}

glm::vec3 Model::getScale()
{
	return m_scale;
}