#pragma once

#include <vulkan/vulkan.h>

class ImageView
{
public:
	virtual VkImageView getImageView() = 0;
};