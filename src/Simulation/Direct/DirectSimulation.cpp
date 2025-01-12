#include "DirectSimulation.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	const uint32_t WORKGROUP_SIZE = 64;

	// Structs
	struct PushConstants {
		float simulationTime;
		float gravitationalConst;
		float softeningLenSqr;
		uint32_t particleCount;
	};

	// Shader source
	const uint32_t SHADER_SOURCE[] {
#include "Shaders/SimShader.comp.u32"
	};

	// Public functions
	size_t DirectSimulation::GetRequiredParticleAlignment() {
		return WORKGROUP_SIZE;
	}

	DirectSimulation::DirectSimulation(VulkanDevice* device, ParticleSystem* particleSystem) : device(device), particleSystem(particleSystem) {
		// Set the descriptor set layout bindings
		VkDescriptorSetLayoutBinding setLayoutBindings[] {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			}
		};

		// Set the descriptor set layout info
		VkDescriptorSetLayoutCreateInfo setLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 3,
			.pBindings = setLayoutBindings
		};

		// Create the descriptor set layout
		VkResult result = vkCreateDescriptorSetLayout(device->GetDevice(), &setLayoutInfo, nullptr, &setLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffer descriptor set layout! Error code: %s", string_VkResult(result));
		
		// Set the descriptor pool size
		VkDescriptorPoolSize descriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 9
		};

		// Set the descriptor pool create info
		VkDescriptorPoolCreateInfo descriptorPoolInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = 3,
			.poolSizeCount = 1,
			.pPoolSizes = &descriptorPoolSize
		};

		// Create the descriptor pool
		result = vkCreateDescriptorPool(device->GetDevice(), &descriptorPoolInfo, nullptr, &descriptorPool);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan compute pipeline descriptor pool! Error code: %s", string_VkResult(result));
		
		// Set the descriptor set alloc info
		VkDescriptorSetLayout setLayouts[] { setLayout, setLayout, setLayout };

		VkDescriptorSetAllocateInfo descriptorSetInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = 3,
			.pSetLayouts = setLayouts
		};

		// Allocate the descriptor sets
		result = vkAllocateDescriptorSets(device->GetDevice(), &descriptorSetInfo, descriptorSets);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle buffer descriptor sets! Error code: %s", string_VkResult(result));
		
		// Set the descriptor buffer infos
		VkDescriptorBufferInfo descriptorBufferInfos[9];
		for(size_t i = 0, ind = 0; i != 3; ++i) {
			descriptorBufferInfos[ind].buffer = particleSystem->GetBuffers()[i].posBuffer;
			descriptorBufferInfos[ind].offset = 0;
			descriptorBufferInfos[ind].range = VK_WHOLE_SIZE;
			++ind;
			descriptorBufferInfos[ind].buffer = particleSystem->GetBuffers()[i].velBuffer;
			descriptorBufferInfos[ind].offset = 0;
			descriptorBufferInfos[ind].range = VK_WHOLE_SIZE;
			++ind;
			descriptorBufferInfos[ind].buffer = particleSystem->GetBuffers()[i].massBuffer;
			descriptorBufferInfos[ind].offset = 0;
			descriptorBufferInfos[ind].range = VK_WHOLE_SIZE;
			++ind;
		}

		// Set the descriptor set writes
		VkWriteDescriptorSet descriptorSetWrites[9];
		for(size_t i = 0, ind = 0; i != 3; ++i) {
			for(size_t j = 0; j != 3; ++j, ++ind) {
				descriptorSetWrites[ind].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorSetWrites[ind].pNext = nullptr;
				descriptorSetWrites[ind].dstSet = descriptorSets[i];
				descriptorSetWrites[ind].dstBinding = j;
				descriptorSetWrites[ind].dstArrayElement = 0;
				descriptorSetWrites[ind].descriptorCount = 1;
				descriptorSetWrites[ind].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorSetWrites[ind].pImageInfo = nullptr;
				descriptorSetWrites[ind].pBufferInfo = descriptorBufferInfos + ind;
				descriptorSetWrites[ind].pTexelBufferView = nullptr;
			}
		}

		// Update the descriptor sets
		vkUpdateDescriptorSets(device->GetDevice(), 9, descriptorSetWrites, 0, nullptr);
		
		// Set the shader module create info
		VkShaderModuleCreateInfo shaderModuleInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(SHADER_SOURCE),
			.pCode = SHADER_SOURCE
		};

		// Create the shader module
		result = vkCreateShaderModule(device->GetDevice(), &shaderModuleInfo, nullptr, &shaderModule);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan simulation shader module! Error code: %s", string_VkResult(result));
		
		// Set the push constant range
		VkPushConstantRange pushConstantRange {
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset = 0,
			.size = sizeof(PushConstants)
		};

		// Set the pipeline layout create info
		VkPipelineLayoutCreateInfo layoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 2,
			.pSetLayouts = setLayouts,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};

		// Create the pipeline layout
		result = vkCreatePipelineLayout(device->GetDevice(), &layoutInfo, nullptr, &pipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan compute pipeline layout! Error code: %s", string_VkResult(result));
		
		// Set the specialization map entry
		VkSpecializationMapEntry specializationEntry {
			.constantID = 0,
			.offset = 0,
			.size = sizeof(uint32_t)
		};

		// Set the specialization info
		VkSpecializationInfo specializationInfo {
			.mapEntryCount = 1,
			.pMapEntries = &specializationEntry,
			.dataSize = sizeof(uint32_t),
			.pData = &WORKGROUP_SIZE
		};
		
		// Set the pipeline create info
		VkComputePipelineCreateInfo pipelineInfo {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = shaderModule,
				.pName = "main",
				.pSpecializationInfo = &specializationInfo
			},
			.layout = pipelineLayout,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		// Create the pipeline
		result = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan simulation compute pipeline! Error code: %s", string_VkResult(result));
		
		// Set the fence create info
		VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		// Create the fence
		result = vkCreateFence(device->GetDevice(), &fenceInfo, nullptr, &simulationFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan simulation synchronization fence! Error code: %s", string_VkResult(result));
		
		// Set the command buffer alloc info
		VkCommandBufferAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = device->GetComputeCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 2
		};

		// Allocate the command buffers
		result = vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, commandBuffers);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan simulation command buffers! Error code: %s", string_VkResult(result));
	}

	void DirectSimulation::RunSimulations(uint32_t simulationCount) {
		// Set the new command buffer index
		commandBufferIndex ^= 1;
		VkCommandBuffer commandBuffer = commandBuffers[commandBufferIndex];

		// Reset the command buffer
		VkResult result = vkResetCommandBuffer(commandBuffer, 0);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to reset Vulkan simulation command buffer! Error code: %s", string_VkResult(result));
		
		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan simulation command buffer! Error code: %s", string_VkResult(result));
		
		// Bind the compute pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

		// Set the push constants
		PushConstants pushConstants {
			.simulationTime = particleSystem->GetSimulationTime(),
			.gravitationalConst = particleSystem->GetGravitationalConst(),
			.softeningLenSqr = particleSystem->GetSofteningLen() * particleSystem->GetSofteningLen(),
			.particleCount = (uint32_t)particleSystem->GetAlignedParticleCount()
		};

		// Set the memory barrier info
		VkMemoryBarrier memoryBarrier {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
		};

		// Record every simulation
		for(uint32_t i = 0; i != simulationCount; ++i) {
			// Bind the descriptor sets
			VkDescriptorSet commandSets[] { descriptorSets[particleSystem->GetComputeInputIndex()], descriptorSets[particleSystem->GetComputeOutputIndex()] };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, commandSets, 0, nullptr);

			// Push the constants
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

			// Run the shader
			vkCmdDispatch(commandBuffer, pushConstants.particleCount / WORKGROUP_SIZE, 1, 1);

			// Add the pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Get the new indices
			particleSystem->NextComputeIndices();
		}

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to end recording Vulkan simulation command buffer! Error code: %s", string_VkResult(result));

		// Wait for the simulation fence
		result = vkWaitForFences(device->GetDevice(), 1, &simulationFence, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to wait for Vulkan simulation fence! Error code: %s", string_VkResult(result));
		
		// Reset the simulation fence
		result = vkResetFences(device->GetDevice(), 1, &simulationFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to reset Vulkan simulation fence! Error code: %s", string_VkResult(result));

		// Set the submit info
		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffers + commandBufferIndex,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		// Submit the command buffer to the compute queue
		result = vkQueueSubmit(device->GetComputeQueue(), 1, &submitInfo, simulationFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to submit Vulkan simulation command buffer! Error code: %s", string_VkResult(result));
	}

	DirectSimulation::~DirectSimulation() {
		// Wait for the simulation fence
		vkWaitForFences(device->GetDevice(), 1, &simulationFence, VK_TRUE, UINT64_MAX);

		// Destroy the pipeline's objects
		vkFreeCommandBuffers(device->GetDevice(), device->GetComputeCommandPool(), 2, commandBuffers);
		vkDestroyFence(device->GetDevice(), simulationFence, nullptr);
		vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
		vkDestroyShaderModule(device->GetDevice(), shaderModule, nullptr);
		vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), setLayout, nullptr);
	}
}