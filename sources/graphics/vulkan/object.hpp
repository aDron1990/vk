#pragma once

#include <graphics/vulkan/model.hpp>

class Object
{
public:
	void init(Model& model);

	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);
	void bindMVP(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4& view, const glm::mat4& proj);
	void bindTexture(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set);
	void bindMaterial(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set);
	void bindMesh(VkCommandBuffer commandBuffer);

	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 scale);
	glm::vec3 getPosition();
	glm::vec3 getRotation();
	glm::vec3 getScale();
	glm::mat4 getModelMatrix();

	Material material{};

private:
	bool m_initialized = false;
	Model* m_model;
	UniformBuffer<MVP> m_mvpBuffer;
	UniformBuffer<Material> m_materialBuffer;
	glm::vec3 m_position{};
	glm::vec3 m_rotation{};
	glm::vec3 m_scale{ 1.0f };
};