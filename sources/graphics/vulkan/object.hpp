#pragma once

#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/dub.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/components/transform.hpp"

class Object
{
public:
	~Object();
	void init();
	void destroy();

	template<typename T, typename... Args>
	T& addComponent(Args&&... args)
	{
		return m_ecs->emplace<T>(m_entity, std::forward<Args>(args)...);
	}

	template<typename T>
	T& getComponent()
	{
		return m_ecs->get<T>(m_entity);
	}

	entt::entity getEntity() { return m_entity; };

private:
	bool m_initialized = false;
	entt::registry* m_ecs{};
	entt::entity m_entity{};
};