#include "renderer/renderer.hpp"
#include "load_file.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <print>
#include <set>
#include <array>
#include <algorithm>
#include <limits>
#include <cstdint>

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

Renderer::Renderer(GLFWwindow* window) : m_window{window}
{
	createContext();
	createDevice();
	createCommandPool();
	createSyncObjects();
	createCommandBuffers();
	createRenderPass();
	createTextureImage();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSets();
	createSwapchain();
	createGraphicsPipeline();
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device->getDevice());
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device->getDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_device->getDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(m_device->getDevice(), m_inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(m_device->getDevice(), m_commandPool, nullptr);
	vkDestroyPipeline(m_device->getDevice(), m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device->getDevice(), m_pipelineLayout, nullptr);
	m_swapchain.reset();
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(m_device->getDevice(), m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_device->getDevice(), m_uniformBuffersMemory[i], nullptr);
	}
	vkDestroyImage(m_device->getDevice(), m_textureImage, nullptr);
	vkFreeMemory(m_device->getDevice(), m_textureImageMemory, nullptr);
	vkDestroyDescriptorPool(m_device->getDevice(), m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device->getDevice(), m_descriptorLayout, nullptr);
	vkDestroyBuffer(m_device->getDevice(), m_vertexBuffer, nullptr);
	vkFreeMemory(m_device->getDevice(), m_vertexBufferMemory, nullptr);
	vkDestroyBuffer(m_device->getDevice(), m_indexBuffer, nullptr);
	vkFreeMemory(m_device->getDevice(), m_indexBufferMemory, nullptr);
	vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
	m_device.reset();
	m_context.reset();
}

void Renderer::createContext()
{
	m_context.reset(new Context{});
}

void Renderer::createDevice()
{
	auto surface = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(m_context->getInstance(), m_window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan surface" };

	m_device.reset(new Device{*m_context, surface});
}

void Renderer::createRenderPass()
{
	auto details = m_device->querySwapchainSupport(m_device->getGpu());
	auto format = Swapchain::chooseSwapchainSurfaceFormat(details.formats);
	auto colorAttachment = VkAttachmentDescription{};
	colorAttachment.format = format.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	auto colorAttachmentRef = VkAttachmentReference{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.colorAttachmentCount = 1;

	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	auto createInfo = VkRenderPassCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = &colorAttachment;
	createInfo.attachmentCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = &dependency;
	createInfo.dependencyCount = 1;

	if (vkCreateRenderPass(m_device->getDevice(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan render pass" };
}

void Renderer::createTextureImage()
{
	int width, height, channels;
	auto* pixels = stbi_load("../../resources/textures/statue.jpg", &width, &height, &channels, STBI_rgb_alpha);
	VkDeviceSize size = width * height * 4;
	if (pixels == nullptr)
		throw std::runtime_error{ "failed to load image" };

	auto stagingBuffer = VkBuffer{};
	auto stagingBufferMemory = VkDeviceMemory{};
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory
	);

	void* data;
	vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(size));
	vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);
	stbi_image_free(pixels);

	createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory
	);

	transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void Renderer::createVertexBuffer()
{
	VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

	auto stagingBuffer = VkBuffer{};
	auto stagingBufferMemory = VkDeviceMemory{};
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data{};
	auto a = vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(size));
	vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

	copyBuffer(stagingBuffer, m_vertexBuffer, size);

	vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void Renderer::createIndexBuffer()
{
	VkDeviceSize size = sizeof(indices[0]) * indices.size();

	auto stagingBuffer = VkBuffer{};
	auto stagingBufferMemory = VkDeviceMemory{};
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data{};
	auto a = vkMapMemory(m_device->getDevice(), stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(size));
	vkUnmapMemory(m_device->getDevice(), stagingBufferMemory);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	copyBuffer(stagingBuffer, m_indexBuffer, size);

	vkDestroyBuffer(m_device->getDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_device->getDevice(), stagingBufferMemory, nullptr);
}

void Renderer::createUniformBuffers()
{
	auto size = VkDeviceSize{ sizeof(UniformBufferObject) };

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			m_uniformBuffers[i], m_uniformBuffersMemory[i]);
		if (vkMapMemory(m_device->getDevice(), m_uniformBuffersMemory[i], 0, size, 0, &m_uniformBuffersMapped[i]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to map uniform buffer" };
	}
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	auto memProps = VkPhysicalDeviceMemoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_device->getGpu(), &memProps);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	throw std::runtime_error{ "failed to find suitable memory type!" };
}

void Renderer::createSwapchain()
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	auto extent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	auto createProps = SwapchainProperties{};
	createProps.renderPass = m_renderPass;
	m_swapchain.reset(new Swapchain{*m_device, createProps});
}

void Renderer::createDescriptorSetLayout()
{
	auto uboBind = VkDescriptorSetLayoutBinding{};
	uboBind.binding = 0;
	uboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBind.descriptorCount = 1;
	uboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto createInfo = VkDescriptorSetLayoutCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = &uboBind;
	createInfo.bindingCount = 1;
	if (vkCreateDescriptorSetLayout(m_device->getDevice(), &createInfo, nullptr, &m_descriptorLayout) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor set layout" };
}

void Renderer::createDescriptorPool()
{
	auto poolSize = VkDescriptorPoolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	auto createInfo = VkDescriptorPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pPoolSizes = &poolSize;
	createInfo.poolSizeCount = 1;
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkCreateDescriptorPool(m_device->getDevice(), &createInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create descriptor pool" };
}

void Renderer::createDescriptorSets()
{
	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	auto layouts = std::vector<VkDescriptorSetLayout>(MAX_FRAMES_IN_FLIGHT, m_descriptorLayout);
	auto allocInfo = VkDescriptorSetAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.pSetLayouts = layouts.data();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_device->getDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate descriptor sets" };

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto bufferInfo = VkDescriptorBufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		auto descriptorWrite = VkWriteDescriptorSet{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.descriptorCount = 1;
		vkUpdateDescriptorSets(m_device->getDevice(), 1, &descriptorWrite, 0, nullptr);
	}
}

void Renderer::createGraphicsPipeline()
{
	auto vertexCode = loadFile("../../resources/shaders/shader.vert.spv");
	auto fragmentCode = loadFile("../../resources/shaders/shader.frag.spv");
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

	auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorLayout;
	pipelineLayoutInfo.setLayoutCount = 1;
	if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
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
	createInfo.layout = m_pipelineLayout;
	createInfo.renderPass = m_renderPass;
	createInfo.subpass = 0;
	if (vkCreateGraphicsPipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_graphicsPipeline))
		throw std::runtime_error{ "failed to create vulkan pipeline" };

	vkDestroyShaderModule(m_device->getDevice(), vertexModule, nullptr);
	vkDestroyShaderModule(m_device->getDevice(), fragmentModule, nullptr);
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
	auto createInfo = VkShaderModuleCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.codeSize = code.size();

	auto shaderModule = VkShaderModule{};
	if (vkCreateShaderModule(m_device->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create shader module" };
	
	return shaderModule;
}

void Renderer::createCommandPool()
{
	auto queueFamilyIndices = m_device->findQueueFamilies(m_device->getGpu());

	auto createInfo = VkCommandPoolCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphics.value();

	if (vkCreateCommandPool(m_device->getDevice(), &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vulkan command pool" };
}

void Renderer::createSyncObjects()
{
	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error{ "failed to create vulkan sync objects" };
	}
}

void Renderer::createCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	auto allocInfo = VkCommandBufferAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vulkan command buffers" };
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	auto createInfo = VkBufferCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_device->getDevice(), &createInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create vertex buffer" };

	auto memReq = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(m_device->getDevice(), buffer, &memReq);

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device->getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate vertex buffer memory" };

	vkBindBufferMemory(m_device->getDevice(), buffer, memory, 0);
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	auto createInfo = VkImageCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(m_device->getDevice(), &createInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error{ "failed to create image" };

	auto memReq = VkMemoryRequirements{};
	vkGetImageMemoryRequirements(m_device->getDevice(), image, &memReq);

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device->getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error{ "failed to allocate image memory" };

	vkBindImageMemory(m_device->getDevice(), image, imageMemory, 0);
}

void Renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	auto commandBuffer = beginSingleTimeCommands();

	auto copyRegion = VkBufferCopy{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void Renderer::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	auto commandBuffer = beginSingleTimeCommands();
	
	auto region = VkBufferImageCopy{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};
	vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void Renderer::updateUniformBuffer()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	auto ubo = UniformBufferObject{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(m_uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

VkCommandBuffer Renderer::beginSingleTimeCommands() 
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(m_device->getGraphicsQueue());
	vkFreeCommandBuffers(m_device->getDevice(), m_commandPool, 1, &commandBuffer);
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	auto commandBuffer = beginSingleTimeCommands();
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	auto barrier = VkImageMemoryBarrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error{ "failed to record command buffer" };

	auto clearColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	auto renderPassInfo = VkRenderPassBeginInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapchain->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapchain->getExtent();
	renderPassInfo.pClearValues = &clearColor;
	renderPassInfo.clearValueCount = 1;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchain->getExtent().width);
	viewport.height = static_cast<float>(m_swapchain->getExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	auto scissor = VkRect2D{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain->getExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		throw std::runtime_error{ "failed to end command buffer" };
}

void Renderer::draw()
{
	auto imageIndex = m_swapchain->beginFrame(m_inFlightFences[currentFrame], m_imageAvailableSemaphores[currentFrame]);
	if (imageIndex == UINT32_MAX) return;
	auto commandBuffer = m_commandBuffers[currentFrame];

	vkResetCommandBuffer(commandBuffer, 0);

	recordCommandBuffer(commandBuffer, imageIndex);

	updateUniformBuffer();

	std::initializer_list<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	auto submitInfo = VkSubmitInfo{};
	auto signalSemaphores = { m_renderFinishedSemaphores[currentFrame] };
	auto waitSemaphores = { m_imageAvailableSemaphores[currentFrame] };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitSemaphores = waitSemaphores.begin();
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitDstStageMask = waitStages.begin();
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.begin();
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error{ "failed to submit draw command buffer" };

	m_swapchain->endFrame(imageIndex, m_renderFinishedSemaphores[currentFrame]);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}