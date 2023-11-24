#include "ComputePipelines.hpp"
#include "Vulkan/Core/Allocator.hpp"
#include "Vulkan/Core/CommandPool.hpp"
#include "Vulkan/Core/Device.hpp"
#include "Vulkan/Data/Points.hpp"
#include "Vulkan/Data/SyncObjects.hpp"
#include "Debug/Logger.hpp"
#include "Manager/Parser.hpp"
#include "Manager/ProgramLoop.hpp"
#include "Utils/Point.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	static const uint32_t GRAVITY_SHADER_SOURCE[] {
#include "Shaders/GravityShader.comp.u32"
	};
	static const uint32_t VELOCITY_SHADER_SOURCE[] {
#include "Shaders/VelocityShader.comp.u32"
	};

	// Structs
	struct PushConstants {
		float deltaTime;
		float gravConst;
		uint32_t pointCount;
	};

	// Variables
	static VkImage velocityImage;
	static VkDeviceMemory velocityImageMemory;
	static VkImageView velocityImageView;

	static VkDescriptorSetLayout descriptorSetLayout;
	static VkDescriptorPool descriptorPool;
	static VkDescriptorSet descriptorSets[POINT_BUFFER_COUNT];

	static VkShaderModule gravityShaderModule;
	static VkShaderModule velocityShaderModule;
	static VkPipelineLayout pipelineLayout;
	static VkPipeline gravityPipeline;
	static VkPipeline velocityPipeline;

	static VkCommandBuffer commandBuffer;

	static uint64_t simulationCount = 0;

	// Internal helper functions
	static void AllocCommandBuffer() {
		// Set the command buffer alloc info
		VkCommandBufferAllocateInfo allocInfo;

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.commandPool = GetVulkanComputeCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		// Allocate the command buffer
		VkResult result = vkAllocateCommandBuffers(GetVulkanDevice(), &allocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan command buffer! Error code: %s", string_VkResult(result));
	}
	static void CreateVelocityImage() {
		// Save the compute queue family index
		uint32_t computeQueueFamilyIndex = GetVulkanDeviceQueueFamilyIndices().computeQueueIndex;

		// Set the velocity image create info
		VkImageCreateInfo imageInfo;

		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.pNext = nullptr;
		imageInfo.flags = 0;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
		imageInfo.extent = { (uint32_t)GetPointCount(), (uint32_t)GetPointCount(), 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.queueFamilyIndexCount = 1;
		imageInfo.pQueueFamilyIndices = &computeQueueFamilyIndex;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// Create the velocity image
		VkResult result = vkCreateImage(GetVulkanDevice(), &imageInfo, GetVulkanAllocCallbacks(), &velocityImage);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan velocity storage image! Error code: %s", string_VkResult(result));

		// Get the image's memory requirements
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(GetVulkanDevice(), velocityImage, &memoryRequirements); 

		// Set the memory alloc info
		VkMemoryAllocateInfo allocInfo;

		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindVulkanMemoryType(0, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Allocate the image's memory
		result = vkAllocateMemory(GetVulkanDevice(), &allocInfo, GetVulkanAllocCallbacks(), &velocityImageMemory);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan velocity storage image memory! Error code: %s", string_VkResult(result));
		
		// Bind the image's memory
		result = vkBindImageMemory(GetVulkanDevice(), velocityImage, velocityImageMemory, 0);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to bind Vulkan velocity storage image memory! Error code: %s", string_VkResult(result));
		
		// Set the image view create info
		VkImageViewCreateInfo imageViewInfo;

		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.pNext = nullptr;
		imageViewInfo.flags = 0;
		imageViewInfo.image = velocityImage;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = VK_FORMAT_R32G32_SFLOAT;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		// Create the image view
		result = vkCreateImageView(GetVulkanDevice(), &imageViewInfo, GetVulkanAllocCallbacks(), &velocityImageView);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan velocity storage image view! Error code: %s", string_VkResult(result));

		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo;
		
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to begin recording Vulkan compute command buffer! Error code: %s", string_VkResult(result));
		
		// Set the image memory barrier
		VkImageMemoryBarrier memoryBarrier;

		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		memoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = velocityImage;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memoryBarrier.subresourceRange.baseMipLevel = 0;
		memoryBarrier.subresourceRange.levelCount = 1;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.subresourceRange.layerCount = 1;

		// Add the pipeline barrier
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to begin recording Vulkan point buffer transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Set the command buffer submit info
		VkSubmitInfo submitInfo;

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		// Submit the command buffer
		result = vkQueueSubmit(GetVulkanComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to submit Vulkan velocity image layout transition command buffer! Error code: %s", string_VkResult(result));
	}
	static void CreateDescriptorSetLayout() {
		// Set the buffer descriptor set layout bindings
		VkDescriptorSetLayoutBinding setLayoutBindings[2];

		setLayoutBindings[0].binding = 0;
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setLayoutBindings[0].descriptorCount = 1;
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		setLayoutBindings[0].pImmutableSamplers = nullptr;

		setLayoutBindings[1].binding = 1;
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		setLayoutBindings[1].descriptorCount = 1;
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		setLayoutBindings[1].pImmutableSamplers = nullptr;

		// Set the descriptor set layout create info
		VkDescriptorSetLayoutCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.bindingCount = 2;
		createInfo.pBindings = setLayoutBindings;

		// Create the descriptor set layout
		VkResult result = vkCreateDescriptorSetLayout(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &descriptorSetLayout);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute pipeline point buffer descriptor set layout! Error code: %s", string_VkResult(result));
	}
	static void CreateDescriptorPool() {
		// Set the storage buffer descriptor pool size
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = POINT_BUFFER_COUNT;

		// Set the descriptor pool create info
		VkDescriptorPoolCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.maxSets = POINT_BUFFER_COUNT;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &poolSize;

		// Create the descriptor pool
		VkResult result = vkCreateDescriptorPool(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &descriptorPool);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute pipeline descriptor pool! Error code: %s", string_VkResult(result));
		
		// Set the descriptor set layout array
		VkDescriptorSetLayout setLayouts[POINT_BUFFER_COUNT];
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			setLayouts[i] = descriptorSetLayout;

		// Set the descriptor set alloc info
		VkDescriptorSetAllocateInfo allocInfo;

		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = POINT_BUFFER_COUNT;
		allocInfo.pSetLayouts = setLayouts;

		// Allocate the descriptor sets
		result = vkAllocateDescriptorSets(GetVulkanDevice(), &allocInfo, descriptorSets);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan compute pipeline point buffer descriptor sets! Error code: %s", string_VkResult(result));
		
		// Set the descirptor buffer infos
		VkDescriptorBufferInfo bufferInfos[POINT_BUFFER_COUNT];

		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			bufferInfos[i].buffer = GetPointBuffers()[i];
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = VK_WHOLE_SIZE;
		}

		// Set the descriptor image info
		VkDescriptorImageInfo imageInfo;
		imageInfo.sampler = VK_NULL_HANDLE;
		imageInfo.imageView = velocityImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Set the descriptor set writes
		VkWriteDescriptorSet descriptorWrites[POINT_BUFFER_COUNT << 1];

		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			descriptorWrites[(i << 1)].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[(i << 1)].pNext = nullptr;
			descriptorWrites[(i << 1)].dstSet = descriptorSets[i];
			descriptorWrites[(i << 1)].dstBinding = 0;
			descriptorWrites[(i << 1)].dstArrayElement = 0;
			descriptorWrites[(i << 1)].descriptorCount = 1;
			descriptorWrites[(i << 1)].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[(i << 1)].pImageInfo = nullptr;
			descriptorWrites[(i << 1)].pBufferInfo = bufferInfos + i;
			descriptorWrites[(i << 1)].pTexelBufferView = nullptr;

			descriptorWrites[(i << 1) + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[(i << 1) + 1].pNext = nullptr;
			descriptorWrites[(i << 1) + 1].dstSet = descriptorSets[i];
			descriptorWrites[(i << 1) + 1].dstBinding = 1;
			descriptorWrites[(i << 1) + 1].dstArrayElement = 0;
			descriptorWrites[(i << 1) + 1].descriptorCount = 1;
			descriptorWrites[(i << 1) + 1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[(i << 1) + 1].pImageInfo = &imageInfo;
			descriptorWrites[(i << 1) + 1].pBufferInfo = nullptr;
			descriptorWrites[(i << 1) + 1].pTexelBufferView = nullptr;
		}

		// Update the descriptor sets
		vkUpdateDescriptorSets(GetVulkanDevice(), POINT_BUFFER_COUNT << 1, descriptorWrites, 0, nullptr);
	}
	static void CreateShaderModules() {
		// Set the gravity shader module create info
		VkShaderModuleCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = sizeof(GRAVITY_SHADER_SOURCE);
		createInfo.pCode = GRAVITY_SHADER_SOURCE;

		// Create the gravity shader module
		VkResult result = vkCreateShaderModule(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &gravityShaderModule);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute gravity shader module! Error code: %s", string_VkResult(result));
		
		// Set the velocity shader module create info
		createInfo.codeSize = sizeof(VELOCITY_SHADER_SOURCE);
		createInfo.pCode = VELOCITY_SHADER_SOURCE;

		// Create the velocity shader module
		result = vkCreateShaderModule(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &velocityShaderModule);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute velocity shader module! Error code: %s", string_VkResult(result));
	}
	static void CreatePipelineLayout() {
		// Set the push constant range
		VkPushConstantRange pushRange;
		pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushRange.offset = 0;
		pushRange.size = sizeof(PushConstants);

		// Set the set layout array
		VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout, descriptorSetLayout };

		// Set the pipeline layout create info
		VkPipelineLayoutCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.setLayoutCount = 2;
		createInfo.pSetLayouts = setLayouts;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushRange;

		// Create the pipeline layout
		VkResult result = vkCreatePipelineLayout(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &pipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute pipeline layout! Error code: %s", string_VkResult(result));
	}
	static void CreatePipelines() {
		// Set the gravity pipeline create info
		VkComputePipelineCreateInfo createInfos[2];
		
		createInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		createInfos[0].pNext = nullptr;
		createInfos[0].flags = 0;
		createInfos[0].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfos[0].stage.pNext = nullptr;
		createInfos[0].stage.flags = 0;
		createInfos[0].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfos[0].stage.module = gravityShaderModule;
		createInfos[0].stage.pName = "main";
		createInfos[0].stage.pSpecializationInfo = nullptr;
		createInfos[0].layout = pipelineLayout;
		createInfos[0].basePipelineHandle = VK_NULL_HANDLE;
		createInfos[0].basePipelineIndex = -1;

		// Set the velocity pipeline create info
		createInfos[1].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		createInfos[1].pNext = nullptr;
		createInfos[1].flags = 0;
		createInfos[1].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfos[1].stage.pNext = nullptr;
		createInfos[1].stage.flags = 0;
		createInfos[1].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfos[1].stage.module = velocityShaderModule;
		createInfos[1].stage.pName = "main";
		createInfos[1].stage.pSpecializationInfo = nullptr;
		createInfos[1].layout = pipelineLayout;
		createInfos[1].basePipelineHandle = VK_NULL_HANDLE;
		createInfos[1].basePipelineIndex = -1;

		// Create the pipelines
		VkPipeline pipelines[2];

		VkResult result = vkCreateComputePipelines(GetVulkanDevice(), VK_NULL_HANDLE, 2, createInfos, GetVulkanAllocCallbacks(), pipelines);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute pipelines! Error code: %s", string_VkResult(result));
		
		// Set the pipeline handles
		gravityPipeline = pipelines[0];
		velocityPipeline = pipelines[1];
	}

	// Public functions
	void CreateComputePipelines() {
		AllocCommandBuffer();
		CreateVelocityImage();
		CreateDescriptorSetLayout();
		CreateDescriptorPool();
		CreateShaderModules();
		CreatePipelineLayout();
		CreatePipelines();

		// Wait for the device to idle
		VkResult result = vkDeviceWaitIdle(GetVulkanDevice());
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for device idle! Error code: %s", result);

		GSIM_LOG_INFO("Created Vulkan compute pipelines.");
	}
	void DestroyComputePipelines() {
		// Wait for the device to idle
		VkResult result = vkDeviceWaitIdle(GetVulkanDevice());
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for device idle! Error code: %s", result);

		// Destroy the pipelines and their layouts
		vkDestroyPipeline(GetVulkanDevice(), gravityPipeline, GetVulkanAllocCallbacks());
		vkDestroyPipeline(GetVulkanDevice(), velocityPipeline, GetVulkanAllocCallbacks());
		vkDestroyPipelineLayout(GetVulkanDevice(), pipelineLayout, GetVulkanAllocCallbacks());

		// Destroy the shader modules
		vkDestroyShaderModule(GetVulkanDevice(), gravityShaderModule, GetVulkanAllocCallbacks());
		vkDestroyShaderModule(GetVulkanDevice(), velocityShaderModule, GetVulkanAllocCallbacks());

		// Destroy the descriptor pool and the set layout
		vkDestroyDescriptorPool(GetVulkanDevice(), descriptorPool, GetVulkanAllocCallbacks());
		vkDestroyDescriptorSetLayout(GetVulkanDevice(), descriptorSetLayout, GetVulkanAllocCallbacks());

		// Destroy the velocity image and its components
		vkDestroyImage(GetVulkanDevice(), velocityImage, GetVulkanAllocCallbacks());
		vkDestroyImageView(GetVulkanDevice(), velocityImageView, GetVulkanAllocCallbacks());
		vkFreeMemory(GetVulkanDevice(), velocityImageMemory, GetVulkanAllocCallbacks());
		
		// Free the command buffer
		vkFreeCommandBuffers(GetVulkanDevice(), GetVulkanComputeCommandPool(), 1, &commandBuffer);
	}

	VkPipelineLayout GetVulkanComputePipelineLayout() {
		return pipelineLayout;
	}
	VkPipeline GetVulkanComputeGravityPipeline() {
		return gravityPipeline;
	}
	VkPipeline GetVulkanComputeVelocityPipeline() {
		return velocityPipeline;
	}

	void SimulateGravity() {
		// Reset the compute fence
		VkFence fence = GetVulkanComputeFence();

		VkResult result = vkResetFences(GetVulkanDevice(), 1, &fence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for Vulkan compute fence! Error code: %s", string_VkResult(result));
		
		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo;
		
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to begin recording Vulkan compute command buffer! Error code: %s", string_VkResult(result));
		
		// Get the new simulation count
		uint64_t newSimulationCount = (uint64_t)(GetSimulationElapsedTime() / GetSimulationInterval());

		// Get the number of simulations to be elapsed
		uint64_t currentSimCount = newSimulationCount - simulationCount;

		// Set the memory barrier info
		VkMemoryBarrier memoryBarrier;

		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		// Set the push constants
		PushConstants pushConstants;
		pushConstants.deltaTime = GetSimulationInterval();
		pushConstants.gravConst = GetGravitationalConstant();
		pushConstants.pointCount = (uint32_t)GetPointCount();

		// Get the current indices
		uint32_t srcIndex = GetCurrentComputeBuffer();
		uint32_t dstIndex = AcquireNextComputeBuffer();

		// Run every simulation
		for(uint64_t i = 0; i != currentSimCount; ++i) {
			// Set the descriptor set array
			VkDescriptorSet gravitySets[2]{ descriptorSets[srcIndex], descriptorSets[dstIndex] };

			// Bind the gravity pipeline and the descriptor sets
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gravityPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, gravitySets, 0, nullptr);

			// Push the constants
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

			// Calculate the gravity for every point
			vkCmdDispatch(commandBuffer, (uint32_t)GetPointCount(), (uint32_t)GetPointCount(), 1);

			// Add the first pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Bind the velocity pipeline and the descriptor sets
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, velocityPipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, gravitySets, 0, nullptr);

			// Push the constants
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

			// Apply the velocity of every point
			vkCmdDispatch(commandBuffer, (uint32_t)GetPointCount(), 1, 1);

			// Add the second pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Set the new indices
			srcIndex = dstIndex;
			dstIndex = AcquireNextComputeBuffer();
		}
		
		// Set the new simulation count
		simulationCount = newSimulationCount;

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to end recording Vulkan compute command buffer! Error code: %s", string_VkResult(result));

		// Set the submit info
		VkSubmitInfo submitInfo;

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		// Submit the command buffer
		result = vkQueueSubmit(GetVulkanComputeQueue(), 1, &submitInfo, fence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to submit Vulkan compute command buffer! Error code: %s", string_VkResult(result));
	}

}