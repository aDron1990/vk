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

void Object::init()
{
	assert(!m_initialized);
	m_initialized = true;
	m_ecs = &Locator::getECS();
	m_entity = m_ecs->create();
	addComponent<Transform>();
}


