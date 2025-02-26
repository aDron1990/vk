#pragma once

#include <graphics/vulkan/model.hpp>
#include <graphics/vulkan/dub.hpp>
#include <graphics/vulkan/types.hpp>

class Object
{
public:
	void init(Model& model, DUB<Material>& materialBuffer);

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);
	void bindMesh(VkCommandBuffer commandBuffer);

	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 scale);
	glm::vec3 getPosition();
	glm::vec3 getRotation();
	glm::vec3 getScale();
	glm::mat4 getModelMatrix();

	void setMaterial(const Material& material);
	Material getMaterial();

	uint32_t getMaterialIndex();

private:
	void updateMaterial();

private:
	bool m_initialized = false;
	Model* m_model{};
	DUB<Material>* m_materialBuffer{};
	Material m_material{};
	uint32_t m_materialIndex{};
	glm::vec3 m_position{};
	glm::vec3 m_rotation{};
	glm::vec3 m_scale{ 1.0f };
};