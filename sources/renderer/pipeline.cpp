#include "renderer/pipeline.hpp"
#include "load_file.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>

Pipeline::Pipeline(Device& device, PipelineInfo& info) : m_device{ device }, m_info{ info }, m_uniformBuffer{device}, m_texture{info.texture}
{
	createDescriptorSetLayout();
	createPipeline();
	createDescriptorPool();
	createDescriptorSets();
}

Pipeline::~Pipeline()
{
	vkDestroyPipeline(m_device.getDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device.getDevice(), m_layout, nullptr);
	vkDestroyDescriptorSetLayout(m_device.getDevice(), m_descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_device.getDevice(), m_descriptorPool, nullptr);
}

void Pipeline::createDescriptorSetLayout()
{
	auto uboBind = VkDescriptorSetLayoutBinding{};
	uboBind.binding = 0;
	uboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBind.descriptorCount = 1;
	uboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto samplerBind = VkDescriptorSetLayoutBinding{};
	samplerBind.binding = 1;
	samplerBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBind.descriptorCount = 1;
	samplerBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto bindings = { uboBind, samplerBind };
	auto createInfo = VkDescriptorSetLayoutCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = bindings.begin();
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	if (vkCreateDescriptorSetLayout(m_device.getDevice(), &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor set layout" };
}

void Pipeline::createPipeline()
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
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.setLayoutCount = 1;
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

void Pipeline::createDescriptorPool()
{
	auto poolSizes = std::vector<VkDescriptorPoolSize>(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	auto createInfo = VkDescriptorPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkCreateDescriptorPool(m_device.getDevice(), &createInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor pool" };
}

void Pipeline::createDescriptorSets()
{
	auto layouts = std::vector<VkDescriptorSetLayout>(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
	auto allocInfo = VkDescriptorSetAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.pSetLayouts = layouts.data();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_device.getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate descriptor sets" };

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = m_uniformBuffer.getBuffer(i).getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		auto imageInfo = VkDescriptorImageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_texture.getImageView();
		imageInfo.sampler = m_texture.getSampler();

		auto descriptorWrites = std::vector<VkWriteDescriptorSet>(2);
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].descriptorCount = 1;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].descriptorCount = 1;
		vkUpdateDescriptorSets(m_device.getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
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
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);
}

void Pipeline::updateBuffer(uint32_t currentFrame)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	auto ubo = UniformBufferObject{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(m_uniformBuffer.getBufferPtr(currentFrame), &ubo, sizeof(ubo));
}

VkPipelineLayout Pipeline::getLayout()
{
	return m_layout;
}