#include "graphics/vulkan/object.hpp"
#include "graphics/vulkan/locator.hpp"

void Object::init(Model& model, DUB<Material>& materialBuffer)
{
	assert(!m_initialized);
	m_initialized = true;
	m_model = &model;
	m_materialBuffer = &materialBuffer;
	m_materialIndex = m_materialBuffer->genIndex();
	updateMaterial();
}

void Object::draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout)
{
	assert(m_initialized);
	bindMesh(commandBuffer);
	m_model->draw(commandBuffer, layout);
}

void Object::bindMesh(VkCommandBuffer commandBuffer)
{
	assert(m_initialized);
	m_model->bindMesh(commandBuffer);
}

void Object::updateMaterial()
{
	assert(m_initialized);
	m_materialBuffer->write(m_materialIndex, m_material);
}

void Object::setPosition(glm::vec3 position)
{
	m_position = position;
}

void Object::setRotation(glm::vec3 rotation)
{
	rotation += 180.0f;
	rotation = glm::mod(rotation, glm::vec3{360.0f});
	rotation -= 180.0f;
	m_rotation = rotation;
}

void Object::setScale(glm::vec3 scale)
{
	m_scale = scale;
}

glm::vec3 Object::getPosition()
{
	assert(m_initialized);
	return m_position;
}

glm::vec3 Object::getRotation()
{
	assert(m_initialized);
	return m_rotation;
}

glm::vec3 Object::getScale()
{
	assert(m_initialized);
	return m_scale;
}

glm::mat4 Object::getModelMatrix()
{
	assert(m_initialized);
	auto model = glm::translate(glm::mat4(1.0f), m_position);
	model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, m_scale);
	return model;
}

void Object::setMaterial(const Material& material)
{
	assert(m_initialized);
	m_material = material;
	updateMaterial();
}

Material Object::getMaterial()
{
	assert(m_initialized);
	return m_material;
}

uint32_t Object::getMaterialIndex()
{
	assert(m_initialized);
	return m_materialIndex;
}