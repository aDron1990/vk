#pragma once

#include "renderer/types.hpp"
#include "renderer/texture.hpp"
#include "renderer/mesh.hpp"
#include "renderer/uniform_buffer.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <string>

using MeshPtr = std::shared_ptr<Mesh>;
using TexturePtr = std::shared_ptr<Texture>;

class Model
{
public:
	Model(Device& device, const std::string& modelPath, const std::string& texturePath);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);

private:
	Device& m_device;
	MeshPtr m_mesh;
	TexturePtr m_texture;
};