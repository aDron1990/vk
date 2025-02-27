#pragma	once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/mesh.hpp"

struct ObjectRenderer
{
	Material material{};
	uint32_t materialIndex{};
	MeshPtr mesh{};
};