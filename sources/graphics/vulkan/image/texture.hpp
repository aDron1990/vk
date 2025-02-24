#pragma once

#include "graphics/vulkan/image/sampler.hpp"

class Texture : public Sampler
{
public:
	virtual void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t setId) = 0;
};