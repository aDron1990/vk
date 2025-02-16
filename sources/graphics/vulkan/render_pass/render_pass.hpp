#pragma once

#include <vulkan/vulkan.h>

#include <array>

class Framebuffer;
class RenderPass
{
public:
	virtual void begin(VkCommandBuffer commandBuffer, Framebuffer* framebuffer) = 0;
	virtual void end(VkCommandBuffer commandBuffer) = 0;
	virtual VkRenderPass getRenderPass() = 0;
};