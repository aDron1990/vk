#include "renderer/graphics_pipeline.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <array>
#include <chrono>

GraphicsPipeline::GraphicsPipeline(Device& device, PipelineInfo& info)
	: Pipeline{ device, info }, m_device{device}, m_uniformBuffer {device}, m_texture{info.texture}
{
	createDescriptorSetLayout();
	createPipeline(m_descriptorSetLayout);
	createDescriptorPool();
	createDescriptorSets();
}

GraphicsPipeline::~GraphicsPipeline()
{
	vkDestroyDescriptorSetLayout(m_device.getDevice(), m_descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_device.getDevice(), m_descriptorPool, nullptr);
}

void GraphicsPipeline::createDescriptorSetLayout()
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

void GraphicsPipeline::createDescriptorPool()
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

void GraphicsPipeline::createDescriptorSets()
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

		auto descriptorWrites = std::array<VkWriteDescriptorSet, 2>{};
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

void GraphicsPipeline::bindDescriptorSets(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getLayout(), 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);
}

void GraphicsPipeline::updateBuffer(uint32_t currentFrame)
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