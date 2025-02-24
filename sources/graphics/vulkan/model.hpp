#pragma once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/image/image_texture.hpp"	
#include "graphics/vulkan/mesh.hpp"
#include "graphics/vulkan/uniform_buffer.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <string>

using MeshPtr = std::shared_ptr<Mesh>;
using TexturePtr = std::shared_ptr<ImageTexture>;

class Model
{
public:
	void init(const std::string& modelPath, const std::string& texturePath);
	void bindTexture(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set);
	void bindMesh(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);

private:
	bool m_initialized = false;
	Device* m_device{};
	MeshPtr m_mesh{};
	TexturePtr m_texture{};
};