#include "BarnesHutSimulation.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	const uint32_t WORKGROUP_SIZE_TREE = 128;
	const uint32_t WORKGROUP_COUNT_TREE = 128;
	const uint32_t WORKGROUP_SIZE_FORCE = 64;

	// Structs
	struct GSIM_ALIGNAS(sizeof(Vec2)) SimulationState {
		Vec4 box;
		Vec4 boxes[WORKGROUP_COUNT_TREE];
		int32_t semaphore;
	};
	struct SpecializationConstants {
		uint32_t workgroupSizeTree;
		uint32_t workgroupCountTree;
		uint32_t workgroupSizeForce;

		float simulationTime;
		float gravitationalConst;
		float softeningLenSqr;

		uint32_t particleCount;
	};

	// Shader sources
	const uint32_t BOX_SHADER_SOURCE[] {
#include "Shaders/BoxShader.comp.u32"
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
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
			.size = (particleSystem->GetAlignedParticleCount() << 1) * sizeof(Vec4i),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
			.size = (particleSystem->GetAlignedParticleCount() << 1) * sizeof(Vec4),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &computeIndex
		};
			
		// Create the box buffer
		result = vkCreateBuffer(device->GetDevice(), &boxBufferInfo, nullptr, &boxBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation box buffer! Error code: %s", string_VkResult(result));
		
		// Get the buffer memory requirements	
		VkMemoryRequirements stateMemRequirements, treeMemRequirements, boxMemRequirements;

		vkGetBufferMemoryRequirements(device->GetDevice(), stateBuffer, &stateMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), treeBuffer, &treeMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), boxBuffer, &boxMemRequirements);

		// Get the max alignment
		VkDeviceSize alignment = stateMemRequirements.alignment;
		if(treeMemRequirements.alignment > alignment)
			alignment = treeMemRequirements.alignment;
		if(boxMemRequirements.alignment > alignment)
			alignment = boxMemRequirements.alignment;
		
		// Align all buffer sizes to the max alignment
		stateMemRequirements.size = (stateMemRequirements.size + alignment - 1) & ~(alignment - 1);
		treeMemRequirements.size = (treeMemRequirements.size + alignment - 1) & ~(alignment - 1);
		boxMemRequirements.size = (boxMemRequirements.size + alignment - 1) & ~(alignment - 1);

		// Get the memory type index
		uint32_t memoryTypeBits = stateMemRequirements.memoryTypeBits & treeMemRequirements.memoryTypeBits & boxMemRequirements.memoryTypeBits;
		uint32_t memoryTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);
		if(memoryTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan Barnes-Hut simulation buffers!");
		
		// Set the alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = stateMemRequirements.size + treeMemRequirements.size + boxMemRequirements.size,
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
			}
		};

		// Set the Barnes-Hut descriptor set layout info
		VkDescriptorSetLayoutCreateInfo barnesHutSetLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 3,
			.pBindings = barnesHutSetLayoutBindings
		};

		// Create the Barnes-Hut descriptor set layout info
		result = vkCreateDescriptorSetLayout(device->GetDevice(), &barnesHutSetLayoutInfo, nullptr, &barnesHutSetLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut buffer descriptor set layout! Error code: %s", string_VkResult(result));

		// Set the descriptor pool size
		VkDescriptorPoolSize descriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 12
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
		VkDescriptorBufferInfo descriptorBufferInfos[12];
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

		descriptorBufferInfos[10].buffer = stateBuffer;
		descriptorBufferInfos[10].offset = 0;
		descriptorBufferInfos[10].range = VK_WHOLE_SIZE;

		descriptorBufferInfos[11].buffer = treeBuffer;
		descriptorBufferInfos[11].offset = 0;
		descriptorBufferInfos[11].range = VK_WHOLE_SIZE;

		descriptorBufferInfos[12].buffer = boxBuffer;
		descriptorBufferInfos[12].offset = 0;
		descriptorBufferInfos[12].range = VK_WHOLE_SIZE;

		// Set the descriptor set writes
		VkWriteDescriptorSet descriptorSetWrites[12];
		for(size_t i = 0, ind = 0; i != 4; ++i) {
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
		vkUpdateDescriptorSets(device->GetDevice(), 12, descriptorSetWrites, 0, nullptr);
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
	}
	void BarnesHutSimulation::CreatePipelines() {
		// Set the pipeline layout info
		VkDescriptorSetLayout setLayouts[] { particleSetLayout, barnesHutSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 2,
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
			.workgroupSizeForce = WORKGROUP_SIZE_FORCE,
			.simulationTime = particleSystem->GetSimulationTime() * particleSystem->GetSimulationSpeed(),
			.gravitationalConst = particleSystem->GetGravitationalConst(),
			.softeningLenSqr = particleSystem->GetSofteningLen() * particleSystem->GetSofteningLen(),
			.particleCount = (uint32_t)particleSystem->GetAlignedParticleCount()
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
				.offset = offsetof(SpecializationConstants, particleCount),
				.size = sizeof(uint32_t)
			}
		};

		// Set the specialization info
		VkSpecializationInfo specializationInfo {
			.mapEntryCount = 7,
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
			}
		};

		// Create the pipelines
		VkPipeline pipelines[1];

		result = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, pipelineInfos, nullptr, pipelines);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut compute pipelines! Error code: %s", string_VkResult(result));
		
		boxPipeline = pipelines[0];
	}

	// Public functions
	size_t BarnesHutSimulation::GetRequiredParticleAlignment() {
		return WORKGROUP_SIZE_TREE;
	}

	BarnesHutSimulation::BarnesHutSimulation(VulkanDevice* device, ParticleSystem* particleSystem) : device(device), particleSystem(particleSystem) {
		// Create all components
		CreateBuffers();
		CreateDescriptorPool();
		CreateShaderModules();
		CreatePipelines();
	}

	BarnesHutSimulation::~BarnesHutSimulation() {
		// Destroy all pipelines and the pipeline layout
		vkDestroyPipeline(device->GetDevice(), boxPipeline, nullptr);

		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);

		// Destroy all shader modules
		vkDestroyShaderModule(device->GetDevice(), boxShader, nullptr);

		// Destroy the descriptor pool and all its components
		vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), particleSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), barnesHutSetLayout, nullptr);

		// Destroy all created buffers and free the used memory
		vkDestroyBuffer(device->GetDevice(), stateBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), treeBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), boxBuffer, nullptr);
		vkFreeMemory(device->GetDevice(), bufferMemory, nullptr);
	}
}