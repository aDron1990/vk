#pragma once

#include <vulkan/vulkan.h>

#include <array>

class RenderPass
{
public:
	virtual void begin(VkCommandBuffer commandBuffer) = 0;
	virtual void end(VkCommandBuffer commandBuffer) = 0;
};