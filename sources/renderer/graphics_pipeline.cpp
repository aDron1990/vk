#include "renderer/graphics_pipeline.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <array>
#include <chrono>
#include <print>

GraphicsPipeline::GraphicsPipeline(Device& device, PipelineInfo& info)
	: Pipeline{ device, info }, m_device{ device }
{
	createPipeline({ m_device.getUBOLayout(), m_device.getSamplerLayout() });
}

GraphicsPipeline::~GraphicsPipeline()
{
	
}

