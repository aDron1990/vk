#pragma once

#include "graphics/vulkan/descriptor/descriptor_pool.hpp"
#include "graphics/vulkan/image/texture.hpp"

#include <string>
#include <vector>
#include <unordered_map>

class TextureArray
{
public:
	~TextureArray();
	void init(uint32_t size);
	void destroy();

	int addTexture(const std::string& imagePath, const std::string& resourceName);
	int findIndex(const std::string& resourceName);
	void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId);

private:
	void create();
	void fillDescriptor(std::shared_ptr<Texture> texture, uint32_t index);

private:
	bool m_initialized = false;
	Device* m_device{};
	DescriptorSetPtr m_descriptorSet{};
	int m_nextIndex{};
	std::vector<std::shared_ptr<Texture>> m_textures;
	std::unordered_map<std::string, int> m_textureIndexes;
};