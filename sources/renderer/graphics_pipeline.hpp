#pragma once

#include "renderer/pipeline.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

class GraphicsPipeline : public Pipeline
{
public:
	GraphicsPipeline(Device& device, PipelineInfo& info);
	~GraphicsPipeline() override;

private:
	Device& m_device;

};