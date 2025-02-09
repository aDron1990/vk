#include "renderer/model.hpp"

Model::Model(Device& device, MeshPtr mesh, TexturePtr texture, std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets, VkDescriptorPool pool)
	: m_device{ device }, m_mesh{ mesh }, m_texture{ texture }, m_descriptorSets{ descriptorSets }, m_pool{pool}
{

}

Model::~Model()
{
	vkFreeDescriptorSets(m_device.getDevice(), m_pool, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data());
}

void Model::draw(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
	m_mesh->bindBuffers(commandBuffer);
	m_mesh->draw(commandBuffer);
}

std::vector<VkDescriptorSet> Model::getDescriptorSets(uint32_t currentFrame)
{
	return { m_descriptorSets[currentFrame] };
}