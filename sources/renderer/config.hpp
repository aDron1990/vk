#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#ifdef _DEBUG
static const bool USE_VALIDATION_LAYERS = true;
#else
static const bool USE_VALIDATION_LAYERS = true;
#endif

static const std::vector<const char*> VALIDATION_LAYER_NAMES =
{
	"VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> DEVICE_EXTENSIONS =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};