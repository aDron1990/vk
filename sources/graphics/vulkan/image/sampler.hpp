#pragma	once

#include "graphics/vulkan/image/image_view.hpp"

class Sampler : public ImageView
{
public:
	virtual VkSampler getSampler() = 0;
};