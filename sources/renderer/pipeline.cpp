#include "renderer/pipeline.hpp"
#include "load_file.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>

Pipeline::Pipeline(Device& device, PipelineInfo& info) : m_device{ device }, m_info{ info }
{}

Pipeline::~Pipeline()
{
	vkDestroyPipeline(m_device.getDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device.getDevice(), m_layout, nullptr);
}

void Pipeline::createPipeline(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	auto vertexCode = loadFile(m_info.vertexPath);
	auto fragmentCode = loadFile(m_info.fragmentPath);
	auto vertexModule = createShaderModule(vertexCode);
	auto fragmentModule = createShaderModule(fragmentCode);

	auto vertexStageInfo = VkPipelineShaderStageCreateInfo{};
	vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStageInfo.module = vertexModule;
	vertexStageInfo.pName = "main";

	auto fragmentStageInfo = VkPipelineShaderStageCreateInfo{};
	fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentStageInfo.module = fragmentModule;
	fragmentStageInfo.pName = "main";

	auto shaderStages = { vertexStageInfo, fragmentStageInfo };

	auto dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	auto dynamicInfo = VkPipelineDynamicStateCreateInfo{};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = dynamicStates.begin();
	dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

	auto viewportState = VkPipelineViewportStateCreateInfo{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	auto bindDesc = Vertex::getBindDesc();
	auto attrDesc = Vertex::getAttrDesc();

	auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());

	auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	auto multisampling = VkPipelineMultisampleStateCreateInfo{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	auto blendingAttachment = VkPipelineColorBlendAttachmentState{};
	blendingAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	blendingAttachment.blendEnable = VK_FALSE;

	auto blending = VkPipelineColorBlendStateCreateInfo{};
	blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blending.logicOpEnable = VK_FALSE;
	blending.attachmentCount = 1;
	blending.pAttachments = &blendingAttachment;

	auto depthStencil = VkPipelineDepthStencilStateCreateInfo{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	if (vkCreatePipelineLayout(m_device.getDevice(), &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan pipeline layout" };

	auto createInfo = VkGraphicsPipelineCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pStages = shaderStages.begin();
	createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pInputAssemblyState = &inputAssembly;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pMultisampleState = &multisampling;
	createInfo.pDepthStencilState = nullptr;
	createInfo.pColorBlendState = &blending;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.layout = m_layout;
	createInfo.renderPass = m_info.renderPass;
	createInfo.subpass = 0;
	if (vkCreateGraphicsPipelines(m_device.getDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline))
		throw std::runtime_error{ "failed to create vulkan pipeline" };

	vkDestroyShaderModule(m_device.getDevice(), vertexModule, nullptr);
	vkDestroyShaderModule(m_device.getDevice(), fragmentModule, nullptr);
}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code)
{
	auto shaderModule = VkShaderModule{};
	auto createInfo = VkShaderModuleCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.codeSize = code.size();
	if (vkCreateShaderModule(m_device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create shader module" };

	return shaderModule;
}

void Pipeline::bind(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

VkPipelineLayout Pipeline::getLayout()
{
	return m_layout;
}