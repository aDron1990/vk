#pragma once

#include "renderer/types.hpp"
#include "renderer/texture.hpp"
#include "renderer/mesh.hpp"

#include <vulkan/vulkan.h>

#include <memory>

using MeshPtr = std::shared_ptr<Mesh>;
using TexturePtr = std::shared_ptr<Texture>;

class Model
{
public:
	Model(Device& device, MeshPtr mesh, TexturePtr texture, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets, VkDescriptorPool pool);
	~Model();
	void draw(VkCommandBuffer commandBuffer, uint32_t currentFrame);
	std::vector<VkDescriptorSet> getDescriptorSets(uint32_t currentFrame);

private:
	Device& m_device;
	MeshPtr m_mesh;
	TexturePtr m_texture;
	MVPBuffer m_mvpBuffer;
	MVP m_mvp;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets;
	const VkDescriptorPool m_pool;
};