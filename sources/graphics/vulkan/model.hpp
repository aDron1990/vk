#pragma once

#include "graphics/vulkan/types.hpp"
#include "graphics/vulkan/mesh.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <string>

class Model
{
public:
	void init(const std::string& modelPath);
	void bindMesh(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout layout);

private:
	bool m_initialized = false;
	Device* m_device{};
	MeshPtr m_mesh{};

};