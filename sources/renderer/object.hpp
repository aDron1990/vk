#pragma once

#include <renderer/model.hpp>

class Object
{
public:
	Object(Device& device, Model& model);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t currentFrame, const glm::mat4& view, const glm::mat4& proj);
	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 scale);
	glm::vec3 getPosition();
	glm::vec3 getRotation();
	glm::vec3 getScale();

private:
	Model& m_model;
	UniformBuffer<MVP> m_mvpBuffer;
	glm::vec3 m_position{};
	glm::vec3 m_rotation{};
	glm::vec3 m_scale{ 1.0f };
};