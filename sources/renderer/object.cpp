#include "renderer/object.hpp"

Object::Object(Device& device, Model& model) : m_model{model}, m_mvpBuffer{device}
{}

void Object::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t currentFrame, const glm::mat4& view, const glm::mat4& proj)
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
	m_mvpBuffer.bind(commandBuffer, layout, currentFrame);
	m_model.draw(commandBuffer, layout, currentFrame);
}

void Object::setPosition(glm::vec3 position)
{
	m_position = position;
}

void Object::setRotation(glm::vec3 rotation)
{
	m_rotation = rotation;
}

void Object::setScale(glm::vec3 scale)
{
	m_scale = scale;
}

glm::vec3 Object::getPosition()
{
	return m_position;
}

glm::vec3 Object::getRotation()
{
	return m_rotation;
}

glm::vec3 Object::getScale()
{
	return m_scale;
}