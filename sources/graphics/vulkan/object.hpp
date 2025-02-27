#pragma once

#include "graphics/vulkan/model.hpp"
#include "graphics/vulkan/dub.hpp"
#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/components/transform.hpp"

class Object
{
public:
	~Object();
	void init(Model& model, DUB<Material>& materialBuffer);
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

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);
	void bindMesh(VkCommandBuffer commandBuffer);

private:
	bool m_initialized = false;
	entt::registry* m_ecs{};
	entt::entity m_entity{};
	Model* m_model{};
	DUB<Material>* m_materialBuffer{};
	Material m_material{};
	uint32_t m_materialIndex{};
};