#pragma once

#include <vulkan/vulkan.h>

class RenderPass
{
public:
	virtual ~RenderPass() = default;
	virtual void begin(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;
	virtual void end(VkCommandBuffer commandBuffer) = 0;
	virtual VkImageView getImageView() = 0;

};