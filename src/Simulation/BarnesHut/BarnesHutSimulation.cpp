#include "BarnesHutSimulation.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	const uint32_t WORKGROUP_SIZE_TREE = 128;
	const uint32_t WORKGROUP_COUNT_TREE = 128;

	// Structs
	struct GSIM_ALIGNAS(sizeof(Vec2)) SimulationState {
		Vec4 box;
		Vec4 boxes[WORKGROUP_COUNT_TREE];

		int32_t semaphore;
		int32_t treeTop;
	};
	struct SpecializationConstants {
		uint32_t workgroupSizeTree;
		uint32_t workgroupCountTree;
		uint32_t workgroupSizeForce;

		float simulationTime;
		float gravitationalConst;
		float softeningLenSqr;
		float accuracyParameter;

		int32_t particleCount;
		int32_t bufferSize;
	};

	// Shader sources
	const uint32_t BOX_SHADER_SOURCE[] {
#include "Shaders/BoxShader.comp.u32"
	};
	const uint32_t TREE_SHADER_SOURCE[] {
#include "Shaders/TreeShader.comp.u32"
	};
	const uint32_t CENTER_SHADER_SOURCE[] {
#include "Shaders/CenterShader.comp.u32"
	};
	const uint32_t SORT_SHADER_SOURCE[] {
#include "Shaders/SortShader.comp.u32"
	};

	// Internal helper functions
	void BarnesHutSimulation::CreateBuffers() {
		// Get the compute family index
		uint32_t computeIndex = device->GetQueueFamilyIndices().computeIndex;

		// Set the state buffer create info
		VkBufferCreateInfo stateBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = sizeof(SimulationState),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &computeIndex
		};

		// Create the state buffer
		VkResult result = vkCreateBuffer(device->GetDevice(), &stateBufferInfo, nullptr, &stateBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation state buffer! Error code: %s", string_VkResult(result));
		
		// Set the tree buffer create info
		VkBufferCreateInfo treeBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = particleSystem->GetBufferSize() * sizeof(Vec4i),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &computeIndex
		};
		
		// Create the tree buffer
		result = vkCreateBuffer(device->GetDevice(), &treeBufferInfo, nullptr, &treeBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation tree buffer! Error code: %s", string_VkResult(result));

		
		// Set the box buffer create info
		VkBufferCreateInfo boxBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = particleSystem->GetBufferSize() * sizeof(Vec4),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &computeIndex
		};
		
		// Create the box buffer
		result = vkCreateBuffer(device->GetDevice(), &boxBufferInfo, nullptr, &boxBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation box buffer! Error code: %s", string_VkResult(result));
		
		// Set the interval buffer create info
		VkBufferCreateInfo intBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = particleSystem->GetBufferSize() * sizeof(Vec2i),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &computeIndex
		};
		
		// Create the box buffer
		result = vkCreateBuffer(device->GetDevice(), &intBufferInfo, nullptr, &intBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation interval buffer! Error code: %s", string_VkResult(result));
		
		// Get the buffer memory requirements	
		VkMemoryRequirements stateMemRequirements, treeMemRequirements, boxMemRequirements, intMemRequirements;

		vkGetBufferMemoryRequirements(device->GetDevice(), stateBuffer, &stateMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), treeBuffer, &treeMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), boxBuffer, &boxMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), intBuffer, &intMemRequirements);

		// Get the max alignment
		VkDeviceSize alignment = stateMemRequirements.alignment;
		if(treeMemRequirements.alignment > alignment)
			alignment = treeMemRequirements.alignment;
		if(boxMemRequirements.alignment > alignment)
			alignment = boxMemRequirements.alignment;
		if(intMemRequirements.alignment > alignment)
			alignment = intMemRequirements.alignment;
		
		// Align all buffer sizes to the max alignment
		stateMemRequirements.size = (stateMemRequirements.size + alignment - 1) & ~(alignment - 1);
		treeMemRequirements.size = (treeMemRequirements.size + alignment - 1) & ~(alignment - 1);
		boxMemRequirements.size = (boxMemRequirements.size + alignment - 1) & ~(alignment - 1);
		intMemRequirements.size = (intMemRequirements.size + alignment - 1) & ~(alignment - 1);

		// Get the memory type index
		uint32_t memoryTypeBits = stateMemRequirements.memoryTypeBits & treeMemRequirements.memoryTypeBits & boxMemRequirements.memoryTypeBits & intMemRequirements.memoryTypeBits;
		uint32_t memoryTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);
		if(memoryTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan Barnes-Hut simulation buffers!");
		
		// Set the alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = stateMemRequirements.size + treeMemRequirements.size + boxMemRequirements.size + intMemRequirements.size,
			.memoryTypeIndex = memoryTypeIndex
		};

		// Allocate the buffer memory
		result = vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &bufferMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan Barnes-Hut buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the state buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), stateBuffer, bufferMemory, 0);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut state buffer to its memory! Error code: %s", string_VkResult(result));
		
		// Bind the tree buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), treeBuffer, bufferMemory, stateMemRequirements.size);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut tree buffer to its memory! Error code: %s", string_VkResult(result));
		
		// Bind the box buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), boxBuffer, bufferMemory, stateMemRequirements.size + treeMemRequirements.size);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut box buffer to its memory! Error code: %s", string_VkResult(result));
		
		// Bind the interval buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), intBuffer, bufferMemory, stateMemRequirements.size + treeMemRequirements.size + boxMemRequirements.size);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut interval buffer to its memory! Error code: %s", string_VkResult(result));
	}
	void BarnesHutSimulation::CreateDescriptorPool() {
		// Set the paraticle descriptor set layout bindings
		VkDescriptorSetLayoutBinding particleSetLayoutBindings[] {
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

		// Set the particle descriptor set layout info
		VkDescriptorSetLayoutCreateInfo particleSetLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 3,
			.pBindings = particleSetLayoutBindings
		};

		// Create the particle descriptor set layout
		VkResult result = vkCreateDescriptorSetLayout(device->GetDevice(), &particleSetLayoutInfo, nullptr, &particleSetLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffer descriptor set layout! Error code: %s", string_VkResult(result));
		
		// Set the Barnes-Hut descriptor set layout bindings
		VkDescriptorSetLayoutBinding barnesHutSetLayoutBindings[] {
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
			},
			{
				.binding = 3,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			}
		};

		// Set the Barnes-Hut descriptor set layout info
		VkDescriptorSetLayoutCreateInfo barnesHutSetLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 4,
			.pBindings = barnesHutSetLayoutBindings
		};

		// Create the Barnes-Hut descriptor set layout info
		result = vkCreateDescriptorSetLayout(device->GetDevice(), &barnesHutSetLayoutInfo, nullptr, &barnesHutSetLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut buffer descriptor set layout! Error code: %s", string_VkResult(result));

		// Set the descriptor pool size
		VkDescriptorPoolSize descriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 13
		};

		// Set the descriptor pool create info
		VkDescriptorPoolCreateInfo descriptorPoolInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = 4,
			.poolSizeCount = 1,
			.pPoolSizes = &descriptorPoolSize
		};

		// Create the descriptor pool
		result = vkCreateDescriptorPool(device->GetDevice(), &descriptorPoolInfo, nullptr, &descriptorPool);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan compute pipeline descriptor pool! Error code: %s", string_VkResult(result));
		
		// Set the descriptor set alloc info
		VkDescriptorSetLayout setLayouts[] { particleSetLayout, particleSetLayout, particleSetLayout, barnesHutSetLayout };

		VkDescriptorSetAllocateInfo descriptorSetInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = 4,
			.pSetLayouts = setLayouts
		};

		// Allocate the descriptor sets
		result = vkAllocateDescriptorSets(device->GetDevice(), &descriptorSetInfo, descriptorSets);
		if(result != VK_SUCCESS)	
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan compute pipeline descriptor sets! Error code: %s", string_VkResult(result));
		
		// Set the descriptor buffer infos
		VkDescriptorBufferInfo descriptorBufferInfos[13];
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

		descriptorBufferInfos[9].buffer = stateBuffer;
		descriptorBufferInfos[9].offset = 0;
		descriptorBufferInfos[9].range = VK_WHOLE_SIZE;

		descriptorBufferInfos[10].buffer = treeBuffer;
		descriptorBufferInfos[10].offset = 0;
		descriptorBufferInfos[10].range = VK_WHOLE_SIZE;

		descriptorBufferInfos[11].buffer = boxBuffer;
		descriptorBufferInfos[11].offset = 0;
		descriptorBufferInfos[11].range = VK_WHOLE_SIZE;

		descriptorBufferInfos[12].buffer = intBuffer;
		descriptorBufferInfos[12].offset = 0;
		descriptorBufferInfos[12].range = VK_WHOLE_SIZE;

		// Set the descriptor set writes
		VkWriteDescriptorSet descriptorSetWrites[13];
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
		for(size_t i = 0, ind = 9; i != 4; ++i, ++ind) {
			descriptorSetWrites[ind].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorSetWrites[ind].pNext = nullptr;
			descriptorSetWrites[ind].dstSet = descriptorSets[3];
			descriptorSetWrites[ind].dstBinding = i;
			descriptorSetWrites[ind].dstArrayElement = 0;
			descriptorSetWrites[ind].descriptorCount = 1;
			descriptorSetWrites[ind].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorSetWrites[ind].pImageInfo = nullptr;
			descriptorSetWrites[ind].pBufferInfo = descriptorBufferInfos + ind;
			descriptorSetWrites[ind].pTexelBufferView = nullptr;
		}

		// Update the descriptor sets
		vkUpdateDescriptorSets(device->GetDevice(), 13, descriptorSetWrites, 0, nullptr);
	}
	void BarnesHutSimulation::CreateShaderModules() {
		// Set the box shader module info
		VkShaderModuleCreateInfo boxShaderInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(BOX_SHADER_SOURCE),
			.pCode = BOX_SHADER_SOURCE
		};

		// Create the box shader module
		VkResult result = vkCreateShaderModule(device->GetDevice(), &boxShaderInfo, nullptr, &boxShader);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut box shader module! Error code: %s", string_VkResult(result));
		
		// Set the tree shader module info
		VkShaderModuleCreateInfo treeShaderInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(TREE_SHADER_SOURCE),
			.pCode = TREE_SHADER_SOURCE
		};

		// Create the tree shader module
		result = vkCreateShaderModule(device->GetDevice(), &treeShaderInfo, nullptr, &treeShader);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut tree shader module! Error code: %s", string_VkResult(result));

		// Set the center shader module info
		VkShaderModuleCreateInfo centerShaderInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(CENTER_SHADER_SOURCE),
			.pCode = CENTER_SHADER_SOURCE
		};

		// Create the center shader module
		result = vkCreateShaderModule(device->GetDevice(), &centerShaderInfo, nullptr, &centerShader);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut center shader module! Error code: %s", string_VkResult(result));
		
		// Set the sort shader module info
		VkShaderModuleCreateInfo sortShaderInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(SORT_SHADER_SOURCE),
			.pCode = SORT_SHADER_SOURCE
		};

		// Create the sort shader module
		result = vkCreateShaderModule(device->GetDevice(), &sortShaderInfo, nullptr, &sortShader);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut sort shader module! Error code: %s", string_VkResult(result));
	}
	void BarnesHutSimulation::CreatePipelines() {
		// Set the pipeline layout info
		VkDescriptorSetLayout setLayouts[] { particleSetLayout, particleSetLayout, barnesHutSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 3,
			.pSetLayouts = setLayouts,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};

		// Create the pipeline layout
		VkResult result = vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut compite pipeline layout! Error code: %s", string_VkResult(result));
		
		// Set the specialization constants
		SpecializationConstants specializationConst {
			.workgroupSizeTree = WORKGROUP_SIZE_TREE,
			.workgroupCountTree = WORKGROUP_COUNT_TREE,
			.workgroupSizeForce = device->GetSubgroupSize(),
			.simulationTime = particleSystem->GetSimulationTime() * particleSystem->GetSimulationSpeed(),
			.gravitationalConst = particleSystem->GetGravitationalConst(),
			.softeningLenSqr = particleSystem->GetSofteningLen() * particleSystem->GetSofteningLen(),
			.accuracyParameter = particleSystem->GetAccuracyParameter(),
			.particleCount = (int32_t)particleSystem->GetAlignedParticleCount(),
			.bufferSize = (int32_t)particleSystem->GetBufferSize()
		};
	
		// Set the specialization map entries
		VkSpecializationMapEntry specializationEntries[] {
			{
				.constantID = 0,
				.offset = offsetof(SpecializationConstants, workgroupSizeTree),
				.size = sizeof(uint32_t)
			},
			{
				.constantID = 1,
				.offset = offsetof(SpecializationConstants, workgroupCountTree),
				.size = sizeof(uint32_t)
			},
			{
				.constantID = 2,
				.offset = offsetof(SpecializationConstants, workgroupSizeForce),
				.size = sizeof(uint32_t)
			},
			{
				.constantID = 3,
				.offset = offsetof(SpecializationConstants, simulationTime),
				.size = sizeof(float)
			},
			{
				.constantID = 4,
				.offset = offsetof(SpecializationConstants, gravitationalConst),
				.size = sizeof(float)
			},
			{
				.constantID = 5,
				.offset = offsetof(SpecializationConstants, softeningLenSqr),
				.size = sizeof(float)
			},
			{
				.constantID = 6,
				.offset = offsetof(SpecializationConstants, accuracyParameter),
				.size = sizeof(float)
			},
			{
				.constantID = 7,
				.offset = offsetof(SpecializationConstants, particleCount),
				.size = sizeof(int32_t)
			},
			{
				.constantID = 8,
				.offset = offsetof(SpecializationConstants, bufferSize),
				.size = sizeof(int32_t)
			}
		};

		// Set the specialization info
		VkSpecializationInfo specializationInfo {
			.mapEntryCount = 9,
			.pMapEntries = specializationEntries,
			.dataSize = sizeof(SpecializationConstants),
			.pData = &specializationConst
		};
		
		// Set the pipeline create infos
		VkComputePipelineCreateInfo pipelineInfos[] {
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = boxShader,
					.pName = "main",
					.pSpecializationInfo = &specializationInfo
				},
				.layout = pipelineLayout,
				.basePipelineHandle = VK_NULL_HANDLE,
				.basePipelineIndex = -1
			},
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = treeShader,
					.pName = "main",
					.pSpecializationInfo = &specializationInfo
				},
				.layout = pipelineLayout,
				.basePipelineHandle = VK_NULL_HANDLE,
				.basePipelineIndex = -1
			},
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = centerShader,
					.pName = "main",
					.pSpecializationInfo = &specializationInfo
				},
				.layout = pipelineLayout,
				.basePipelineHandle = VK_NULL_HANDLE,
				.basePipelineIndex = -1
			},
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = sortShader,
					.pName = "main",
					.pSpecializationInfo = &specializationInfo
				},
				.layout = pipelineLayout,
				.basePipelineHandle = VK_NULL_HANDLE,
				.basePipelineIndex = -1
			}
		};

		// Create the pipelines
		VkPipeline pipelines[4];

		result = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 4, pipelineInfos, nullptr, pipelines);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut compute pipelines! Error code: %s", string_VkResult(result));
		
		boxPipeline = pipelines[0];
		treePipeline = pipelines[1];
		centerPipeline = pipelines[2];
		sortPipeline = pipelines[3];
	}
	void BarnesHutSimulation::CreateCommandObjects() {
		// Set the simulation fence create info
		VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// Create the simulation fence
		VkResult result = vkCreateFence(device->GetDevice(), &fenceInfo, nullptr, &simulationFence);
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
	void BarnesHutSimulation::ClearBuffers() {
		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		// Begin recording the command buffer
		VkResult result = vkBeginCommandBuffer(commandBuffers[0], &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan simulation command buffer! Error code: %s", string_VkResult(result));
		
		// Clear the buffers
		vkCmdFillBuffer(commandBuffers[0], stateBuffer, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(commandBuffers[0], treeBuffer, 0, VK_WHOLE_SIZE, (uint32_t)-1);
		vkCmdFillBuffer(commandBuffers[0], boxBuffer, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(commandBuffers[0], intBuffer, 0, VK_WHOLE_SIZE, (uint32_t)-1);

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffers[0]);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to end recording Vulkan simulation command buffer! Error code: %s", string_VkResult(result));

		// Set the submit info
		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffers,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		// Submit the command buffer to the compute queue
		result = vkQueueSubmit(device->GetComputeQueue(), 1, &submitInfo, simulationFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to submit Vulkan simulation command buffer! Error code: %s", string_VkResult(result));

		// Wait for the clear to finish
		result = vkWaitForFences(device->GetDevice(), 1, &simulationFence, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to wait for Vulkan simulation fence! Error code: %s", string_VkResult(result));
	}

	// Public functions
	size_t BarnesHutSimulation::GetRequiredParticleAlignment() {
		return 1;
	}

	BarnesHutSimulation::BarnesHutSimulation(VulkanDevice* device, ParticleSystem* particleSystem) : device(device), particleSystem(particleSystem) {
		// Create all components
		CreateBuffers();
		CreateDescriptorPool();
		CreateShaderModules();
		CreatePipelines();
		CreateCommandObjects();
		ClearBuffers();
	}

	void BarnesHutSimulation::RunSimulations(uint32_t simulationCount) {
		// Exit the function if no simulations will be recorded
		if(!simulationCount)
			return;

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
			VkDescriptorSet commandSets[] { descriptorSets[particleSystem->GetComputeInputIndex()], descriptorSets[particleSystem->GetComputeOutputIndex()], descriptorSets[3] };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 3, commandSets, 0, nullptr);

			// Run the box shader
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, boxPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_COUNT_TREE, 1, 1);

			// Add a pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Run the tree shader
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, treePipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_COUNT_TREE, 1, 1);

			// Add a pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Run the center shader
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, centerPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_COUNT_TREE, 1, 1);

			// Add a pipeline barrier
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Run the sort shader
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, sortPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_COUNT_TREE, 1, 1);

			// Add a pipeline barrier
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

	BarnesHutSimulation::~BarnesHutSimulation() {
		// Wait for the simulation fence
		vkWaitForFences(device->GetDevice(), 1, &simulationFence, VK_TRUE, UINT64_MAX);

		// Destroy the command objects
		vkFreeCommandBuffers(device->GetDevice(), device->GetComputeCommandPool(), 2, commandBuffers);
		vkDestroyFence(device->GetDevice(), simulationFence, nullptr);

		// Destroy all pipelines and the pipeline layout
		vkDestroyPipeline(device->GetDevice(), boxPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), treePipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), centerPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), sortPipeline, nullptr);

		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);

		// Destroy all shader modules
		vkDestroyShaderModule(device->GetDevice(), boxShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), treeShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), centerShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), sortShader, nullptr);

		// Destroy the descriptor pool and all its components
		vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), particleSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), barnesHutSetLayout, nullptr);

		// Destroy all created buffers and free the used memory
		vkDestroyBuffer(device->GetDevice(), stateBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), treeBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), boxBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), intBuffer, nullptr);

		vkFreeMemory(device->GetDevice(), bufferMemory, nullptr);
	}
}