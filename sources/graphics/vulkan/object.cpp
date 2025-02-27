#include "graphics/vulkan/object.hpp"
#include "graphics/vulkan/locator.hpp"

Object::~Object()
{
	destroy();
}

void Object::destroy()
{
	if (m_initialized)
	{
		m_ecs->destroy(m_entity);
	}
	m_initialized = false;
}

void Object::init(Model& model, DUB<Material>& materialBuffer)
{
	assert(!m_initialized);
	m_initialized = true;
	m_ecs = &Locator::getECS();
	m_entity = m_ecs->create();
	m_model = &model;
	m_materialBuffer = &materialBuffer;
	m_materialIndex = m_materialBuffer->genIndex();
	addComponent<Transform>();
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