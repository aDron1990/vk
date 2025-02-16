#pragma once

#include "graphics/vulkan/config.hpp"
#include "graphics/vulkan/device.hpp"
#include "graphics/vulkan/render_pass/render_pass.hpp"
#include "graphics/vulkan/render_pass/framebuffer_props.hpp"
#include "graphics/vulkan/uniform_buffer.hpp"
#include "graphics/vulkan/texture.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

struct PipelineProps
{
	std::string vertexPath;
	std::string fragmentPath;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	bool vertexInput;
	VkCullModeFlags culling;
};

class Pipeline
{
public:
	Pipeline();
	~Pipeline();
	void init(const PipelineProps& props, const FramebufferProps& framebufferProps, RenderPass& renderPass);
	void destroy();

	void bind(VkCommandBuffer commandBuffer);
	VkPipelineLayout getLayout();
	
protected:
	void createPipeline();

private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

private:
	Device& m_device;
	RenderPass* m_renderPass{};
	PipelineProps m_props{};
	FramebufferProps m_framebufferProps{};
	bool m_initialized = false;
	VkPipelineLayout m_layout{};
	VkPipeline m_pipeline{};
};