#pragma once

#include "graphics/vulkan/image/image_view.hpp"
#include "graphics/vulkan/context/device.hpp"
#include "graphics/vulkan/descriptor/descriptor_pool.hpp"

class SwapchainImage : public ImageView
{
public:
	~SwapchainImage();
	void init(VkImage swapchainImage, VkFormat format);
	void destroy();
	VkImageView getImageView() override;

private:
	void createImageView(VkImageAspectFlags aspect);

private:
	bool m_initialized = false;
	Device* m_device;
	VkFormat m_format{};
	VkImage m_image{};
	VkImageView m_imageView{};
};