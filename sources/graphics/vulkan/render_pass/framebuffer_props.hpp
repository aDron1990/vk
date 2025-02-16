#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

struct FramebufferProps
{
	uint32_t width;
	uint32_t height;
	uint32_t colorAttachmentCount;
	bool useDepthAttachment;
	VkFormat colorFormat;
	VkFormat depthFormat;
	//bool storeDepthAttachment;
};