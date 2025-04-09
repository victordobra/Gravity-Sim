#include "BarnesHutSimulation.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	const uint32_t WORKGROUP_SIZE_BOX = 128;
	const uint32_t WORKGROUP_SIZE_TREE = 8;

	// Structs
	struct GSIM_ALIGNAS(sizeof(Vec2)) SimulationState {
		Vec4 box;
		Vec4 boxes[WORKGROUP_SIZE_BOX];
	};
	struct SpecializationConstants {
		uint32_t workgroupSizeBox;
		uint32_t workgroupSizeTree;
		uint32_t workgroupSizeForce;

		float simulationTime;
		float gravitationalConst;
		float softeningLenSqr;
		float accuracyParameterSqr;

		uint32_t particleCount;
	};

	// Shader sources
	const uint32_t BOX_SHADER_1_SOURCE[] {
#include "Shaders/BoxShader1.comp.u32"
	};
	const uint32_t BOX_SHADER_2_SOURCE[] {
#include "Shaders/BoxShader2.comp.u32"
	};
	const uint32_t CLEAR_SHADER_SOURCE[] {
#include "Shaders/ClearShader.comp.u32"
	};
	const uint32_t INIT_SHADER_SOURCE[] {
#include "Shaders/InitShader.comp.u32"
	};

	// Internal helper functions
	void BarnesHutSimulation::CreateBuffers() {
		// Get the compute family index
		uint32_t computeIndex = device->GetQueueFamilyIndices().computeIndex;

		// Save the buffer sizes to an array
		VkDeviceSize bufferSizes[] {
			sizeof(SimulationState),                            // stateBuffer
			sizeof(uint32_t) * particleSystem->GetBufferSize(), // countBuffer
			sizeof(float) * particleSystem->GetBufferSize(),    // radiusBuffer
			sizeof(uint32_t) * particleSystem->GetBufferSize(), // srcBuffer
		};

		// Create the buffers and get their infos
		VkBuffer buffers[4];
		VkMemoryRequirements memRequirements[4];
		VkDeviceSize alignment = 1;
		uint32_t memoryTypeBits = 0xffffffffu;

		for(uint32_t i = 0; i != 4; ++i) {
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
		for(uint32_t i = 0; i != 4; ++i) {
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
		for(uint32_t i = 0; i != 4; ++i) {
			result = vkBindBufferMemory(device->GetDevice(), buffers[i], bufferMemory, offset);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut simulation buffers to their memory! Error code: %s", string_VkResult(result));

			offset += memRequirements[i].size;
		}

		// Assign all buffers
		stateBuffer = buffers[0];
		countBuffer = buffers[1];
		radiusBuffer = buffers[2];
		srcBuffer = buffers[3];
	}
	void BarnesHutSimulation::CreateImages() {
		// Get the compute family index
		uint32_t computeIndex = device->GetQueueFamilyIndices().computeIndex;

		// Save the image formats in an array
		VkFormat imageFormats[] {
			VK_FORMAT_R32_UINT,      // treeCountImage
			VK_FORMAT_R32G32_UINT,   // treeStartImage
			VK_FORMAT_R32G32_SFLOAT, // treePosImage
			VK_FORMAT_R32_SFLOAT     // treeMassImage
		};

		// Create the images and get their infos
		VkImage images[4];
		VkMemoryRequirements memRequirements[4];
		VkDeviceSize alignment = 1;
		uint32_t memoryTypeBits = 0xffffffffu;

		for(uint32_t i = 0; i != 4; ++i) {
			// Set the image's info
			VkImageCreateInfo imageInfo {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = imageFormats[i],
				.extent = { 512, 512, 1 },
				.mipLevels = 10,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_STORAGE_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &computeIndex,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};

			// Create the image
			VkResult result = vkCreateImage(device->GetDevice(), &imageInfo, nullptr, images + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation storage images! Error code: %s", string_VkResult(result));
			
			// Get the image's memory requirements
			vkGetImageMemoryRequirements(device->GetDevice(), images[i], memRequirements + i);

			// Set the global memory's new info
			if(memRequirements[i].alignment)
				alignment = memRequirements[i].alignment;
			memoryTypeBits &= memRequirements[i].memoryTypeBits;
		}

		// Set the allocated memory's size
		VkDeviceSize memorySize = 0;
		for(uint32_t i = 0; i != 4; ++i) {
			memRequirements[i].size = (memRequirements[i].size + alignment - 1) & ~(alignment - 1);
			memorySize += memRequirements[i].size;
		}

		// Get the memory type's index
		uint32_t memoryTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryTypeBits);
		if(memoryTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan Barnes-Hut simulation storage images!");
		
		// Set the alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memorySize,
			.memoryTypeIndex = memoryTypeIndex
		};

		// Allocate the image memory
		VkResult result = vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &imageMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan Barnes-Hut storage image memory! Error code: %s", string_VkResult(result));
		
		// Bind the images to their memory
		VkDeviceSize offset = 0;
		for(uint32_t i = 0; i != 4; ++i) {
			result = vkBindImageMemory(device->GetDevice(), images[i], imageMemory, offset);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan Barnes-Hut simulation storage images to their memory! Error code: %s", string_VkResult(result));

			offset += memRequirements[i].size;
		}

		// Create the image views
		VkImageView imageViews[4][10];

		for(uint32_t i = 0; i != 4; ++i) {
			for(uint32_t j = 0; j != 10; ++j) {
				// Set the image view info
				VkImageViewCreateInfo imageViewInfo {
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.image = images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = imageFormats[i],
					.components = {
						.r = VK_COMPONENT_SWIZZLE_IDENTITY,
						.g = VK_COMPONENT_SWIZZLE_IDENTITY,
						.b = VK_COMPONENT_SWIZZLE_IDENTITY,
						.a = VK_COMPONENT_SWIZZLE_IDENTITY
					},
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = j,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				};

				// Create the image view
				result = vkCreateImageView(device->GetDevice(), &imageViewInfo, nullptr, &imageViews[i][j]);
				if(result != VK_SUCCESS)
					GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation storage image views! Error code: %s", string_VkResult(result));
			}
		}

		// Save the created images and views
		treeCountImage = images[0];
		treeStartImage = images[1];
		treePosImage = images[2];
		treeMassImage = images[3];

		for(uint32_t i = 0; i != 10; ++i)
			treeCountImageViews[i] = imageViews[0][i];
		for(uint32_t i = 0; i != 10; ++i)
			treeStartImageViews[i] = imageViews[1][i];
		for(uint32_t i = 0; i != 10; ++i)
			treePosImageViews[i] = imageViews[2][i];
		for(uint32_t i = 0; i != 10; ++i)
			treeMassImageViews[i] = imageViews[3][i];
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
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 10,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 5,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 10,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 6,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 10,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			},
			{
				.binding = 7,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 10,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr
			}
		};

		// Set the Barnes-Hut descriptor set layout info
		VkDescriptorSetLayoutCreateInfo barnesHutSetLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 8,
			.pBindings = barnesHutSetLayoutBindings
		};

		// Create the Barnes-Hut descriptor set layout
		result = vkCreateDescriptorSetLayout(device->GetDevice(), &barnesHutSetLayoutInfo, nullptr, &barnesHutSetLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut buffer descriptor set layout! Error code: %s", string_VkResult(result));
		
		// Set the descriptor pool sizes
		VkDescriptorPoolSize descriptorPoolSizes[] {
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 13
			},
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 40
			}
		};

		// Set the descriptor pool create info
		VkDescriptorPoolCreateInfo descriptorPoolInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = 4,
			.poolSizeCount = 2,
			.pPoolSizes = descriptorPoolSizes
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
			stateBuffer, countBuffer, radiusBuffer, srcBuffer
		};

		VkDescriptorBufferInfo bufferInfos[13];
		for(uint32_t i = 0; i != 13; ++i) {
			bufferInfos[i] = {
				.buffer = buffers[i],
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};
		}

		// Set the descriptor image infos
		VkImageView* imageViews[] { treeCountImageViews, treeStartImageViews, treePosImageViews, treeMassImageViews };

		VkDescriptorImageInfo imageInfos[40];

		for(uint32_t i = 0, ind = 0; i != 4; ++i) {
			for(uint32_t j = 0; j != 10; ++j, ++ind) {
				imageInfos[ind] = {
					.sampler = VK_NULL_HANDLE,
					.imageView = imageViews[i][j],
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL
				};
			}
		}

		// Set the descriptor set writes
		VkWriteDescriptorSet setWrites[53];

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
		for(uint32_t i = 0, ind = 9; i != 4; ++i, ++ind) {
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
		for(uint32_t i = 0, ind = 13; i != 4; ++i) {
			for(uint32_t j = 0; j != 10; ++j, ++ind) {
				setWrites[ind] = {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = descriptorSets[3],
					.dstBinding = i + 4,
					.dstArrayElement = j,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.pImageInfo = imageInfos + (i * 10 + j),
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			}
		}

		// Update the descriptor sets
		vkUpdateDescriptorSets(device->GetDevice(), 53, setWrites, 0, nullptr);
	}
	void BarnesHutSimulation::CreateShaderModules() {
		// Save the shader sources and source sizes to arrays
		const uint32_t* shaderSources[] {
			BOX_SHADER_1_SOURCE,
			BOX_SHADER_2_SOURCE,
			CLEAR_SHADER_SOURCE,
			INIT_SHADER_SOURCE
		};
		size_t shaderSourceSizes[] {
			sizeof(BOX_SHADER_1_SOURCE),
			sizeof(BOX_SHADER_2_SOURCE),
			sizeof(CLEAR_SHADER_SOURCE),
			sizeof(INIT_SHADER_SOURCE)
		};

		// Create all shader modules
		VkShaderModule shaders[4];

		for(uint32_t i = 0; i != 4; ++i) {
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
		boxShader1 = shaders[0];
		boxShader2 = shaders[1];
		clearShader = shaders[2];
		initShader = shaders[3];
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
			.setLayoutCount = 3,
			.pSetLayouts = setLayouts,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};

		// Create the tree pipeline layout
		result = vkCreatePipelineLayout(device->GetDevice(), &treePipelineLayoutInfo, nullptr, &treePipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation pipeline layouts! Error code: %s", string_VkResult(result));

		// Set the specialization constants
		SpecializationConstants specializationConst {
			.workgroupSizeBox = WORKGROUP_SIZE_BOX,
			.workgroupSizeTree = WORKGROUP_SIZE_TREE,
			.workgroupSizeForce = device->GetSubgroupSize(),
			.simulationTime = particleSystem->GetSimulationTime() * particleSystem->GetSimulationSpeed(),
			.gravitationalConst = particleSystem->GetGravitationalConst(),
			.softeningLenSqr = particleSystem->GetSofteningLen() * particleSystem->GetSofteningLen(),
			.accuracyParameterSqr = particleSystem->GetAccuracyParameter() * particleSystem->GetAccuracyParameter(),
			.particleCount = (uint32_t)particleSystem->GetAlignedParticleCount()
		};

		// Set the specialization map entries
		VkSpecializationMapEntry specializationEntries[] {
			{
				.constantID = 0,
				.offset = offsetof(SpecializationConstants, workgroupSizeBox),
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
				.offset = offsetof(SpecializationConstants, particleCount),
				.size = sizeof(int32_t)
			}
		};

		// Set the specialization info
		VkSpecializationInfo specializationInfo {
			.mapEntryCount = 8,
			.pMapEntries = specializationEntries,
			.dataSize = sizeof(SpecializationConstants),
			.pData = &specializationConst
		};
		
		// Save the shader modules and pipeline layouts in arrays
		VkShaderModule shaders[] {
			boxShader1,
			boxShader2,
			clearShader,
			initShader
		};
		VkPipelineLayout pipelineLayouts[] {
			bufferPipelineLayout, // boxPipeline1
			bufferPipelineLayout, // boxPipeline2
			bufferPipelineLayout, // clearPipeline
			bufferPipelineLayout  // initPipeline
		};

		// Set the pipeline create infos
		VkComputePipelineCreateInfo pipelineInfos[4];
		for(uint32_t i = 0; i != 4; ++i) {
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
		VkPipeline pipelines[4];
		result = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 4, pipelineInfos, nullptr, pipelines);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan Barnes-Hut simulation pipelines! Error code: %s", string_VkResult(result));
		
		// Save the created pipelines
		boxPipeline1 = pipelines[0];
		boxPipeline2 = pipelines[1];
		clearPipeline = pipelines[2];
		initPipeline = pipelines[3];
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
	void BarnesHutSimulation::SetImageLayouts() {
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
		
		// Save the images to an array
		VkImage images[] {
			treeCountImage,
			treeStartImage,
			treePosImage,
			treeMassImage
		};

		// Set the memory barriers
		VkImageMemoryBarrier barriers[4];
		for(uint32_t i = 0; i != 4; ++i) {
			barriers[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = images[i],
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 10,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};
		}

		// Apply the layout transition
		vkCmdPipelineBarrier(commandBuffers[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 4, barriers);

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
		CreateImages();
		CreateDescriptorPool();
		CreateShaderModules();
		CreatePipelines();
		CreateCommandObjects();
		SetImageLayouts();
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
			vkCmdDispatch(commandBuffer, 1024 / WORKGROUP_SIZE_TREE, 1024 / WORKGROUP_SIZE_TREE, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Calculate the bounding box
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, boxPipeline1);
			vkCmdDispatch(commandBuffer, WORKGROUP_SIZE_BOX, 1, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, boxPipeline2);
			vkCmdDispatch(commandBuffer, 1, 1, 1);
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

			// Write the particle datas into the tree
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, initPipeline);
			vkCmdDispatch(commandBuffer, WORKGROUP_SIZE_BOX, 1, 1);
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

		// Destroy the pipelines and their layouts
		vkDestroyPipeline(device->GetDevice(), boxPipeline1, nullptr);
		vkDestroyPipeline(device->GetDevice(), boxPipeline2, nullptr);
		vkDestroyPipeline(device->GetDevice(), clearPipeline, nullptr);
		vkDestroyPipeline(device->GetDevice(), initPipeline, nullptr);

		vkDestroyPipelineLayout(device->GetDevice(), bufferPipelineLayout, nullptr);
		vkDestroyPipelineLayout(device->GetDevice(), treePipelineLayout, nullptr);

		// Destroy the shader modules
		vkDestroyShaderModule(device->GetDevice(), boxShader1, nullptr);
		vkDestroyShaderModule(device->GetDevice(), boxShader2, nullptr);
		vkDestroyShaderModule(device->GetDevice(), clearShader, nullptr);
		vkDestroyShaderModule(device->GetDevice(), initShader, nullptr);

		// Destroy the descriptor pool and its set layouts
		vkDestroyDescriptorPool(device->GetDevice(), descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), particleSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device->GetDevice(), barnesHutSetLayout, nullptr);

		// Destroy the images and their image views and free the image memory
		vkDestroyImage(device->GetDevice(), treeCountImage, nullptr);
		vkDestroyImage(device->GetDevice(), treeStartImage, nullptr);
		vkDestroyImage(device->GetDevice(), treePosImage, nullptr);
		vkDestroyImage(device->GetDevice(), treeMassImage, nullptr);

		for(uint32_t i = 0; i != 10; ++i)
			vkDestroyImageView(device->GetDevice(), treeCountImageViews[i], nullptr);
		for(uint32_t i = 0; i != 10; ++i)
			vkDestroyImageView(device->GetDevice(), treeStartImageViews[i], nullptr);
		for(uint32_t i = 0; i != 10; ++i)
			vkDestroyImageView(device->GetDevice(), treePosImageViews[i], nullptr);
		for(uint32_t i = 0; i != 10; ++i)
			vkDestroyImageView(device->GetDevice(), treeMassImageViews[i], nullptr);
		
		vkFreeMemory(device->GetDevice(), imageMemory, nullptr);

		// Destroy the buffers and free the buffer memory
		vkDestroyBuffer(device->GetDevice(), stateBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), countBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), radiusBuffer, nullptr);
		vkDestroyBuffer(device->GetDevice(), srcBuffer, nullptr);

		vkFreeMemory(device->GetDevice(), bufferMemory, nullptr);
	}
}