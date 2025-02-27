#include "graphics/vulkan/image/texture_array.hpp"
#include "graphics/vulkan/image/image_texture.hpp"
#include "graphics/vulkan/locator.hpp"

#include <cassert>

TextureArray::~TextureArray()
{
	destroy();
}

void TextureArray::destroy()
{
	if (m_initialized)
	{
		m_textures.clear();
	}
	m_initialized = false;
}

void TextureArray::init(uint32_t size)
{
	assert(!m_initialized);
	m_initialized = true;
	m_device = &Locator::getDevice();
	m_textures.resize(size);
	m_descriptorSet = Locator::getDescriptorPool().createSet(3);
	create();
}

void TextureArray::create()
{
	assert(m_initialized);
	auto texture = std::make_shared<ImageTexture>();
	texture->init("resources/images/error.png", false);
	for (int i = 0; i < m_textures.size(); i++)
	{
		fillDescriptor(texture, i);
	}
	m_textures.push_back(texture);
	m_nextIndex++;
}

int TextureArray::addTexture(const std::string& imagePath, const std::string& resourceName)
{
	assert(m_initialized);
	assert(m_nextIndex < m_textures.size());

	auto texture = std::make_shared<ImageTexture>();
	texture->init(imagePath);

	fillDescriptor(texture, m_nextIndex);

	m_textures.push_back(texture);
	m_textureIndexes[resourceName] = m_nextIndex;
	return m_nextIndex++;
}

int TextureArray::findIndex(const std::string& resourceName)
{
	return m_textureIndexes[resourceName];
}

void TextureArray::fillDescriptor(std::shared_ptr<Texture> texture, uint32_t index)
{
	auto imageInfo = VkDescriptorImageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture->getImageView();
	imageInfo.sampler = texture->getSampler();

	auto descriptorWrite = VkWriteDescriptorSet{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = m_descriptorSet->getSet();
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = index;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void TextureArray::bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId)
{
	assert(m_initialized);
	auto set = m_descriptorSet->getSet();
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, setId, 1, &set, 0, nullptr);
}