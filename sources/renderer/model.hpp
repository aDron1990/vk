#pragma once

#include "renderer/types.hpp"
#include "renderer/texture.hpp"
#include "renderer/mesh.hpp"
#include "renderer/uniform_buffer.hpp"

#include <vulkan/vulkan.h>

#include <memory>

using MeshPtr = std::shared_ptr<Mesh>;
using TexturePtr = std::shared_ptr<Texture>;

class Model
{
public:
	Model(Device& device, MeshPtr mesh, TexturePtr texture);
	~Model();
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t currentFrame, const glm::mat4& view, const glm::mat4& proj);
	void setPosition(glm::vec3 position);
	void setRotation(glm::vec3 rotation);
	void setScale(glm::vec3 scale);
	glm::vec3 getPosition();
	glm::vec3 getRotation();
	glm::vec3 getScale();

private:
	Device& m_device;
	MeshPtr m_mesh;
	TexturePtr m_texture;
	UniformBuffer<UniformBufferObject> m_mvpBuffer;
	glm::vec3 m_position{};
	glm::vec3 m_rotation{};
	glm::vec3 m_scale{1.0f};

};