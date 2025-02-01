#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;
};

using FindQueueFamilyFunc = std::function<QueueFamilyIndices(VkPhysicalDevice gpu)>;