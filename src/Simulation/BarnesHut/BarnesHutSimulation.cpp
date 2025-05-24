#include "BarnesHutSimulation.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	const uint32_t WORKGROUP_SIZE_PARTICLE = 128;
	const uint32_t WORKGROUP_SIZE_TREE = 64;

	// Structs
	struct SpecializationConstants {
		uint32_t workgroupSizeParticle;
		uint32_t workgroupSizeTree;
		uint32_t workgroupSizeForce;

		float simulationTime;
		float gravitationalConst;
		float softeningLenSqr;
		float accuracyParameterSqr;
		float simulationSize;

		uint32_t particleCount;
		uint32_t treeSize;
	};

	// Shader sources
	const uint32_t CLEAR_SHADER_SOURCE[] {
#include "Shaders/ClearShader.comp.u32"
	};
	const uint32_t FORCE_SHADER_SOURCE[] {
#include "Shaders/ForceShader.comp.u32"
	};
	const uint32_t INIT_SHADER_SOURCE[] {
#include "Shaders/InitShader.comp.u32"
	};
	const uint32_t PARTICLE_SORT_SHADER_SOURCE[] {
#include "Shaders/ParticleSortShader.comp.u32"
	};
	const uint32_t TREE_INIT_SHADER_SOURCE[] {
#include "Shaders/TreeInitShader.comp.u32"
	};
	const uint32_t TREE_MOVE_SHADER_SOURCE[] {
#include "Shaders/TreeMoveShader.comp.u32"
	};
	const uint32_t TREE_SORT_SHADER_SOURCE[] {
#include "Shaders/TreeSortShader.comp.u32"
	};

	// Internal helper functions
	void BarnesHutSimulation::CreateBuffers() {
		// Get the compute family index
		uint32_t computeIndex = device->GetQueueFamilyIndices().computeIndex;

		// Calculate the required buffer capacity
		VkDeviceSize bufferCap = particleSystem->GetAlignedParticleCount() + 1398101;

		// Save the buffer sizes to an array
		VkDeviceSize bufferSizes[] {
			sizeof(uint32_t) * bufferCap, // countBuffer
			sizeof(float) * bufferCap,    // radiusBuffer
			sizeof(Vec2) * bufferCap,     // nodePosBuffer
			sizeof(float) * bufferCap,    // nodeMassBuffer
			sizeof(uint32_t) * bufferCap  // srcBuffer
		};

		// Create the buffers and get their infos
		VkBuffer buffers[5];
		VkMemoryRequirements memRequirements[5];
		VkDeviceSize alignment = 1;
		uint32_t memoryTypeBits = 0xffffffffu;

		for(uint32_t i = 0; i != 5; ++i) {
			// Set the buffer info
			VkBufferCreateInfo bufferInfo {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.size = bufferSizes[i],
				.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &computeIndex
			};

			// Create the buffer
			VkResult result = vkCreateBuffer(device->GetDevice(), &bufferInfo, nullptr, buffers + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation buffers! Error code: %s", string_VkResult(result));
			
			// Get the buffer's memory requirements
			vkGetBufferMemoryRequirements(device->GetDevice(), buffers[i], memRequirements + i);

			// Set the global memory's new info
			if(memRequirements[i].alignment)
				alignment = memRequirements[i].alignment;
			memoryTypeBits &= memRequirements[i].memoryTypeBits;
		}

		// Set the allocated memory's size
		VkDeviceSize memorySize = 0;
		for(uint32_t i = 0; i != 5; ++i) {
			memRequirements[i].size = (memRequirements[i].size + alignment - 1) & ~(alignment - 1);
			memorySize += memRequirements[i].size;
		}

		// Get the memory type's index
		uint32_t memoryTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);
		if(memoryTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan Barnes-Hut simulation buffers!");
		
		// Set the alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memorySize,
			.memoryTypeIndex = memoryTypeIndex
		};

		// Allocate the buffer memory
		VkResult result = vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &bufferMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan Barnes-Hut buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the buffers to their memory
		VkDeviceSize offset = 0;
		for(uint32_t i = 0; i != 5; ++i) {
			result = vkBindBufferMemory(device->GetDevice(), buffers[i], bufferMemory, offset);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut simulation buffers to their memory! Error code: %s", string_VkResult(result));

			offset += memRequirements[i].size;
		}

		// Assign all buffers
		countBuffer = buffers[0];
		radiusBuffer = buffers[1];
		nodePosBuffer = buffers[2];
		nodeMassBuffer = buffers[3];
		srcBuffer = buffers[4];
	}
	void BarnesHutSimulation::CreateTreeBuffers() {
		// Get the compute family index
		uint32_t computeIndex = device->GetQueueFamilyIndices().computeIndex;

		// Save the buffer format sizes to an array
		VkDeviceSize formatSizes[] {
			sizeof(Vec2u),
			sizeof(Vec2u),
			sizeof(Vec2),
			sizeof(float)
		};

		// Create the buffers and get their infos
		VkBuffer buffers[44];
		VkMemoryRequirements memRequirements[44];
		VkDeviceSize alignment = 1;
		uint32_t memoryTypeBits = 0xffffffffu;

		for(uint32_t i = 0, ind = 0; i != 4; ++i) {
			for(uint32_t j = 0; j != 11; ++j, ++ind) {
				// Set the buffer info
				VkBufferCreateInfo bufferInfo {
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.size = formatSizes[i] * ((VkDeviceSize)1 << (j << 1)),
					.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 1,
					.pQueueFamilyIndices = &computeIndex
				};

				// Create the buffer
				VkResult result = vkCreateBuffer(device->GetDevice(), &bufferInfo, nullptr, buffers + ind);
				if(result != VK_SUCCESS)
					GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation buffers! Error code: %s", string_VkResult(result));
				
				// Get the buffer's memory requirements
				vkGetBufferMemoryRequirements(device->GetDevice(), buffers[ind], memRequirements + ind);

				// Set the global memory's new info
				if(memRequirements[ind].alignment)
					alignment = memRequirements[ind].alignment;
				memoryTypeBits &= memRequirements[ind].memoryTypeBits;
			}
		}

		// Set the allocated memory's size
		VkDeviceSize memorySize = 0;
		for(uint32_t i = 0; i != 44; ++i) {
			memRequirements[i].size = (memRequirements[i].size + alignment - 1) & ~(alignment - 1);
			memorySize += memRequirements[i].size;
		}

		// Get the memory type's index
		uint32_t memoryTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);
		if(memoryTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan Barnes-Hut simulation buffers!");
		
		// Set the alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memorySize,
			.memoryTypeIndex = memoryTypeIndex
		};

		// Allocate the buffer memory
		VkResult result = vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &treeBufferMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan Barnes-Hut buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the buffers to their memory
		VkDeviceSize offset = 0;
		for(uint32_t i = 0; i != 44; ++i) {
			result = vkBindBufferMemory(device->GetDevice(), buffers[i], treeBufferMemory, offset);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut simulation buffers to their memory! Error code: %s", string_VkResult(result));

			offset += memRequirements[i].size;
		}

		// Assign all buffers
		for(uint32_t i = 0; i != 11; ++i)
			treeCountBuffers[i] = buffers[i];
		for(uint32_t i = 0; i != 11; ++i)
			treeStartBuffers[i] = buffers[11 + i];
		for(uint32_t i = 0; i != 11; ++i)
			treePosBuffers[i] = buffers[22 + i];
		for(uint32_t i = 0; i != 11; ++i)
			treeMassBuffers[i] = buffers[33 + i];
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
			},
			{
				.binding = 4,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 5,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 11,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 6,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 11,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 7,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 11,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 8,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 11,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			}
		};

		// Set the Barnes-Hut descriptor set layout info
		VkDescriptorSetLayoutCreateInfo barnesHutSetLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 9,
			.pBindings = barnesHutSetLayoutBindings
		};

		// Create the Barnes-Hut descriptor set layout
		result = vkCreateDescriptorSetLayout(device->GetDevice(), &barnesHutSetLayoutInfo, nullptr, &barnesHutSetLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut buffer descriptor set layout! Error code: %s", string_VkResult(result));
		
		// Set the descriptor pool size
		VkDescriptorPoolSize descriptorPoolSize {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 54
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
		VkBuffer buffers[] {
			particleSystem->GetBuffers()[0].posBuffer, particleSystem->GetBuffers()[0].velBuffer, particleSystem->GetBuffers()[0].massBuffer,
			particleSystem->GetBuffers()[1].posBuffer, particleSystem->GetBuffers()[1].velBuffer, particleSystem->GetBuffers()[1].massBuffer,
			particleSystem->GetBuffers()[2].posBuffer, particleSystem->GetBuffers()[2].velBuffer, particleSystem->GetBuffers()[2].massBuffer,
			countBuffer, radiusBuffer, nodePosBuffer, nodeMassBuffer, srcBuffer
		};

		VkDescriptorBufferInfo bufferInfos[58];
		for(uint32_t i = 0; i != 14; ++i) {
			bufferInfos[i] = {
				.buffer = buffers[i],
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};
		}

		// Set the descriptor tree buffer infos
		VkBuffer* treeBuffers[] {
			treeCountBuffers, treeStartBuffers, treePosBuffers, treeMassBuffers
		};

		for(uint32_t i = 0, ind = 14; i != 4; ++i) {
			for(uint32_t j = 0; j != 11; ++j, ++ind) {
				bufferInfos[ind] = {
					.buffer = treeBuffers[i][j],
					.offset = 0,
					.range = VK_WHOLE_SIZE
				};
			}
		}

		// Set the descriptor set writes
		VkWriteDescriptorSet setWrites[58];

		for(uint32_t i = 0, ind = 0; i != 3; ++i) {
			for(uint32_t j = 0; j != 3; ++j, ++ind) {
				setWrites[ind] = {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = descriptorSets[i],
					.dstBinding = j,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = bufferInfos + ind,
					.pTexelBufferView = nullptr
				};
			}
		}
		for(uint32_t i = 0, ind = 9; i != 5; ++i, ++ind) {
			setWrites[ind] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = descriptorSets[3],
				.dstBinding = i,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = bufferInfos + ind,
				.pTexelBufferView = nullptr
			};
		}
		for(uint32_t i = 0, ind = 14; i != 4; ++i) {
			for(uint32_t j = 0; j != 11; ++j, ++ind) {
				setWrites[ind] = {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = descriptorSets[3],
					.dstBinding = i + 5,
					.dstArrayElement = j,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = bufferInfos + ind,
					.pTexelBufferView = nullptr
				};
			}
		}

		// Update the descriptor sets
		vkUpdateDescriptorSets(device->GetDevice(), 58, setWrites, 0, nullptr);
	}
	void BarnesHutSimulation::CreateShaderModules() {
		// Save the shader sources and source sizes to arrays
		const uint32_t* shaderSources[] {
			CLEAR_SHADER_SOURCE,
			FORCE_SHADER_SOURCE,
			INIT_SHADER_SOURCE,
			PARTICLE_SORT_SHADER_SOURCE,
			TREE_INIT_SHADER_SOURCE,
			TREE_MOVE_SHADER_SOURCE,
			TREE_SORT_SHADER_SOURCE
		};
		size_t shaderSourceSizes[] {
			sizeof(CLEAR_SHADER_SOURCE),
			sizeof(FORCE_SHADER_SOURCE),
			sizeof(INIT_SHADER_SOURCE),
			sizeof(PARTICLE_SORT_SHADER_SOURCE),
			sizeof(TREE_INIT_SHADER_SOURCE),
			sizeof(TREE_MOVE_SHADER_SOURCE),
			sizeof(TREE_SORT_SHADER_SOURCE)
		};

		// Create all shader modules
		VkShaderModule shaders[7];

		for(uint32_t i = 0; i != 7; ++i) {
			// Set the shader module create info
			VkShaderModuleCreateInfo shaderInfo {
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderSourceSizes[i],
				.pCode = shaderSources[i]
			};

			// Create the shader module
			VkResult result = vkCreateShaderModule(device->GetDevice(), &shaderInfo, nullptr, shaders + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut compute shader modules! Error code: %s", string_VkResult(result));
		}

		// Save the created shader modules
		clearShader = shaders[0];
		forceShader = shaders[1];
		initShader = shaders[2];
		particleSortShader = shaders[3];
		treeInitShader = shaders[4];
		treeMoveShader = shaders[5];
		treeSortShader = shaders[6];
	}
	void BarnesHutSimulation::CreatePipelines() {
		// Set the descriptor set layouts
		VkDescriptorSetLayout setLayouts[] { particleSetLayout, particleSetLayout, barnesHutSetLayout };

		// Set the buffer pipeline layout create info
		VkPipelineLayoutCreateInfo bufferPipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 3,
			.pSetLayouts = setLayouts,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};

		// Create the buffer pipeline layout
		VkResult result = vkCreatePipelineLayout(device->GetDevice(), &bufferPipelineLayoutInfo, nullptr, &bufferPipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation pipeline layouts! Error code: %s", string_VkResult(result));
		
		// Set the push constant range
		VkPushConstantRange pushConstantRange {
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset = 0,
			.size = sizeof(uint32_t)
		};

		// Set the tree pipeline layout create info
		VkPipelineLayoutCreateInfo treePipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &barnesHutSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};

		// Create the tree pipeline layout
		result = vkCreatePipelineLayout(device->GetDevice(), &treePipelineLayoutInfo, nullptr, &treePipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation pipeline layouts! Error code: %s", string_VkResult(result));

		// Set the specialization constants
		SpecializationConstants specializationConst {
			.workgroupSizeParticle = WORKGROUP_SIZE_PARTICLE,
			.workgroupSizeTree = WORKGROUP_SIZE_TREE,
			.workgroupSizeForce = device->GetSubgroupSize(),
			.simulationTime = particleSystem->GetSimulationTime() * particleSystem->GetSimulationSpeed(),
			.gravitationalConst = particleSystem->GetGravitationalConst(),
			.softeningLenSqr = particleSystem->GetSofteningLen() * particleSystem->GetSofteningLen(),
			.accuracyParameterSqr = particleSystem->GetAccuracyParameter() * particleSystem->GetAccuracyParameter(),
			.simulationSize = 500,
			.particleCount = (uint32_t)particleSystem->GetAlignedParticleCount(),
			.treeSize = 1024
		};

		// Set the specialization map entries
		VkSpecializationMapEntry specializationEntries[] {
			{
				.constantID = 0,
				.offset = offsetof(SpecializationConstants, workgroupSizeParticle),
				.size = sizeof(uint32_t)
			},
			{
				.constantID = 1,
				.offset = offsetof(SpecializationConstants, workgroupSizeTree),
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
				.offset = offsetof(SpecializationConstants, accuracyParameterSqr),
				.size = sizeof(float)
			},
			{
				.constantID = 7,
				.offset = offsetof(SpecializationConstants, simulationSize),
				.size = sizeof(float)
			},
			{
				.constantID = 8,
				.offset = offsetof(SpecializationConstants, particleCount),
				.size = sizeof(uint32_t)
			},
			{
				.constantID = 9,
				.offset = offsetof(SpecializationConstants, treeSize),
				.size = sizeof(uint32_t)
			}
		};

		// Set the specialization info
		VkSpecializationInfo specializationInfo {
			.mapEntryCount = 10,
			.pMapEntries = specializationEntries,
			.dataSize = sizeof(SpecializationConstants),
			.pData = &specializationConst
		};
		
		// Save the shader modules and pipeline layouts in arrays
		VkShaderModule shaders[] {
			clearShader,
			forceShader,
			initShader,
			particleSortShader,
			treeInitShader,
			treeMoveShader,
			treeSortShader
		};
		VkPipelineLayout pipelineLayouts[] {
			bufferPipelineLayout, // clearPipeline
			bufferPipelineLayout, // forcePipeline
			bufferPipelineLayout, // initPipeline
			bufferPipelineLayout, // particleSortPipeline
			treePipelineLayout,   // treeInitPipeline
			treePipelineLayout,   // treeMovePipeline
			treePipelineLayout    // treeSortPipeline
		};

		// Set the pipeline create infos
		VkComputePipelineCreateInfo pipelineInfos[7];
		for(uint32_t i = 0; i != 7; ++i) {
			pipelineInfos[i] = {
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = shaders[i],
					.pName = "main",
					.pSpecializationInfo = &specializationInfo
				},
				.layout = pipelineLayouts[i],
				.basePipelineHandle =  VK_NULL_HANDLE,
				.basePipelineIndex = -1
			};
		}

		// Create the pipelines
		VkPipeline pipelines[7];
		result = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 7, pipelineInfos, nullptr, pipelines);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation pipelines! Error code: %s", string_VkResult(result));
		
		// Save the created pipelines
		clearPipeline = pipelines[0];
		forcePipeline = pipelines[1];
		initPipeline = pipelines[2];
		particleSortPipeline = pipelines[3];
		treeInitPipeline = pipelines[4];
		treeMovePipeline = pipelines[5];
		treeSortPipeline = pipelines[6];
	}
	void BarnesHutSimulation::CreateCommandObjects() {
		// Set the simulation fence create info
		VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
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
	void BarnesHutSimulation::RecordSecondaryCommandBuffers() {
		// Set the command buffer alloc info
		VkCommandBufferAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = device->GetComputeCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = 1
		};

		// Allocate the command buffer
		VkResult result = vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, &treeCommandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan simulation tree construction command buffer! Error code: %s", string_VkResult(result));
		
		// Set the command buffer inheritance info
		VkCommandBufferInheritanceInfo inheritanceInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
			.pNext = nullptr,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0,
			.framebuffer = VK_NULL_HANDLE,
			.occlusionQueryEnable = VK_FALSE,
			.queryFlags = 0,
			.pipelineStatistics = 0
		};

		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			.pInheritanceInfo = &inheritanceInfo
		};

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(treeCommandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan simulation tree construction command buffer! Error code: %s", string_VkResult(result));

		// Set the memory barrier info
		VkMemoryBarrier memoryBarrier {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
		};

		// Bind the simulation descriptor set
		vkCmdBindDescriptorSets(treeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, treePipelineLayout, 0, 1, descriptorSets + 3, 0, nullptr);

		// Record the tree initiation
		for(uint32_t i = 9; i != UINT32_MAX; --i) {
			// Push the current depth
			vkCmdPushConstants(treeCommandBuffer, treePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &i);

			// Get the number of workgroups
			uint32_t treeSize = 1 << (i << 1);
			uint32_t workgroupCount = (treeSize + WORKGROUP_SIZE_TREE - 1) / WORKGROUP_SIZE_TREE;

			// Build the current depth of the tree
			vkCmdBindPipeline(treeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, treeInitPipeline);
			vkCmdDispatch(treeCommandBuffer, workgroupCount, 1, 1);
			vkCmdPipelineBarrier(treeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		}

		// Record the tree sorting
		for(uint32_t i = 0; i != 10; ++i) {
			// Push the current depth
			vkCmdPushConstants(treeCommandBuffer, treePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &i);

			// Get the number of workgroups
			uint32_t treeSize = 1 << (i << 1);
			uint32_t workgroupCount = (treeSize + WORKGROUP_SIZE_TREE - 1) / WORKGROUP_SIZE_TREE;

			// Sort the current depth of the tree
			vkCmdBindPipeline(treeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, treeSortPipeline);
			vkCmdDispatch(treeCommandBuffer, workgroupCount, 1, 1);
			vkCmdPipelineBarrier(treeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		}

		// Record the tree moving
		for(uint32_t i = 0; i != 11; ++i) {
			// Push the current depth
			vkCmdPushConstants(treeCommandBuffer, treePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &i);

			// Get the number of workgroups
			uint32_t treeSize = 1 << (i << 1);
			uint32_t workgroupCount = (treeSize + WORKGROUP_SIZE_TREE - 1) / WORKGROUP_SIZE_TREE;

			// Move the current depth of the tree
			vkCmdBindPipeline(treeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, treeMovePipeline);
			vkCmdDispatch(treeCommandBuffer, workgroupCount, 1, 1);
		}

		vkCmdPipelineBarrier(treeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

		// End recording the command buffer
		result = vkEndCommandBuffer(treeCommandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to end recording Vulkan simulation tree construction command buffer! Error code: %s", string_VkResult(result));
	}

	// Public functions
	size_t BarnesHutSimulation::GetRequiredParticleAlignment() {
		return 64;
	}

	BarnesHutSimulation::BarnesHutSimulation(VulkanDevice* device, ParticleSystem* particleSystem) : device(device), particleSystem(particleSystem) {
		// Create all components
		CreateBuffers();
		CreateTreeBuffers();
		CreateDescriptorPool();
		CreateShaderModules();
		CreatePipelines();
		CreateCommandObjects();
		RecordSecondaryCommandBuffers();
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
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
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
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
		};

		// Record every simulation
		for(uint32_t i = 0; i != simulationCount; ++i) {
			// Bind the descriptor sets
			VkDescriptorSet commandSets[] { descriptorSets[particleSystem->GetComputeInputIndex()], descriptorSets[particleSystem->GetComputeOutputIndex()], descriptorSets[3] };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bufferPipelineLayout, 0, 3, commandSets, 0, nullptr);

			// Clear the previous tree
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, clearPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_SIZE_PARTICLE, 1, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Write the particle datas into the tree
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, initPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_SIZE_PARTICLE, 1, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Build the tree
			vkCmdExecuteCommands(commandBuffer, 1, &treeCommandBuffer);

			// Rebind the descriptor sets
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bufferPipelineLayout, 0, 3, commandSets, 0, nullptr);

			// Sort the particles
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, particleSortPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_SIZE_PARTICLE, 1, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Calculate and apply the forces
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, forcePipeline);
			vkCmdDispatch(commandBuffer, (uint32_t)(particleSystem->GetAlignedParticleCount() / device->GetSubgroupSize()), 1, 1);
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

		// Free the secondary command buffer
		vkFreeCommandBuffers(device->GetDevice(), device->GetComputeCommandPool(), 1, &treeCommandBuffer);

		// Destroy the command objects
		vkFreeCommandBuffers(device->GetDevice(), device->GetComputeCommandPool(), 2, commandBuffers);
		vkDestroyFence(device->GetDevice(), simulationFence, nullptr);

		// Destroy the pipelines and their layouts
		vkDestroyPipeline(device->GetDevice(), clearPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), forcePipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), initPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), particleSortPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), treeInitPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), treeMovePipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), treeSortPipeline, nullptr);

		vkDestroyPipelineLayout(device->GetDevice(), bufferPipelineLayout, nullptr);
		vkDestroyPipelineLayout(device->GetDevice(), treePipelineLayout, nullptr);

		// Destroy the shader modules
		vkDestroyShaderModule(device->GetDevice(), clearShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), forceShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), initShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), particleSortShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), treeInitShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), treeMoveShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), treeSortShader, nullptr);

		// Destroy the descriptor pool and its set layouts
		vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), particleSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), barnesHutSetLayout, nullptr);

		// Destroy the tree buffers and free the tree buffer memory
		for(uint32_t i = 0; i != 11; ++i)
			vkDestroyBuffer(device->GetDevice(), treeCountBuffers[i], nullptr);
		for(uint32_t i = 0; i != 11; ++i)
			vkDestroyBuffer(device->GetDevice(), treeStartBuffers[i], nullptr);
		for(uint32_t i = 0; i != 11; ++i)
			vkDestroyBuffer(device->GetDevice(), treePosBuffers[i], nullptr);
		for(uint32_t i = 0; i != 11; ++i)
			vkDestroyBuffer(device->GetDevice(), treeMassBuffers[i], nullptr);
		
		vkFreeMemory(device->GetDevice(), treeBufferMemory, nullptr);

		// Destroy the buffers and free the buffer memory
		vkDestroyBuffer(device->GetDevice(), countBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), radiusBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), nodePosBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), nodeMassBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), srcBuffer, nullptr);

		vkFreeMemory(device->GetDevice(), bufferMemory, nullptr);
	}
}