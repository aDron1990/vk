#pragma once

#include "graphics/vulkan/render_pass/framebuffer_props.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/image/render_texture.hpp"

#include <vulkan/vulkan.h>

class RenderPass;
class Framebuffer
{
public:
	virtual VkFramebuffer getFramebuffer() = 0;
	virtual VkExtent2D getExtent() = 0;
};