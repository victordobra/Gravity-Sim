#include "ParticleSystem.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Internal helper functions
	void ParticleSystem::GenerateParticlesRandom(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Set the random seed
		srand(time(nullptr));

		// Generate every particle
		for(size_t i = 0; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar
			float theta = ((float)rand() / RAND_MAX) * 2 * M_PI;
			float r = sqrt((float)rand() / RAND_MAX) * generateSize;

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * cosf(theta), r * sinf(theta) };

			// Set the particle's velocity to zero
			particles[i].vel = { 0, 0 };

			// Generate the particle's mass
			particles[i].mass = ((float)rand() / RAND_MAX) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesGalaxy(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Set the random seed
		srand(time(nullptr));

		// Generate every particle
		for(size_t i = 0; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar
			float theta = ((float)rand() / RAND_MAX) * 2 * M_PI;
			float rNorm = (float)rand() / RAND_MAX;
			float r = rNorm * generateSize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * 0.08f, thetaCos * r * 0.08f };

			// Generate the particle's mass
			particles[i].mass = ((float)rand() / RAND_MAX) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Set the random seed
		srand(time(nullptr));

		// Generate every particle in the first galaxy
		float galaxySize = generateSize / 3;
		for(size_t i = 0; i != particleCount >> 1; ++i) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = ((float)rand() / RAND_MAX) * 2 * M_PI;
			float rNorm = (float)rand() / RAND_MAX;
			float r = rNorm * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos - galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * 0.25f + galaxySize * 0.1f, thetaCos * r * 0.25f };

			// Generate the particle's mass
			particles[i].mass = ((float)rand() / RAND_MAX) * (maxMass - minMass) + minMass;
		}

		// Generate avery particle in the second galaxy
		for(size_t i = particleCount >> 1; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = ((float)rand() / RAND_MAX) * 2 * M_PI;
			float rNorm = (float)rand() / RAND_MAX;
			float r = rNorm * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos + galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * 0.25f - galaxySize * 0.1f, thetaCos * r * 0.25f };

			// Generate the particle's mass
			particles[i].mass = ((float)rand() / RAND_MAX) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesSymmetricalGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Set the random seed
		srand(time(nullptr));

		// Generate every particle in the first galaxy and mirror it in the second
		float galaxySize = generateSize / 3;
		for(size_t i = 0; i != particleCount; i += 2) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = ((float)rand() / RAND_MAX) * 2 * M_PI;
			float rNorm = (float)rand() / RAND_MAX;
			float r = rNorm * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos - galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * 0.25f + galaxySize * 0.1f, thetaCos * r * 0.25f };

			// Generate the particle's mass
			particles[i].mass = ((float)rand() / RAND_MAX) * (maxMass - minMass) + minMass;

			// Mirror the particle from the first galaxy in the second
			particles[i + 1].pos = { -particles[i].pos.x, particles[i].pos.y };
			particles[i + 1].vel = { -thetaSin * r * 0.25f - galaxySize * 0.1f, -thetaCos * r * 0.25f };
			particles[i + 1].mass = particles[i].mass;
		}
	}
	void ParticleSystem::CreateVulkanObjects(const Particle* particles) {
		// Set the staging buffer create info
		uint32_t transferIndex = device->GetQueueFamilyIndices().transferIndex;

		VkBufferCreateInfo stagingBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = (VkDeviceSize)(alignedParticleCount * sizeof(Particle)),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &transferIndex
		};

		// Create the staging buffer
		VkBuffer stagingBuffer;
		VkResult result = vkCreateBuffer(device->GetDevice(), &stagingBufferInfo, nullptr, &stagingBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle staging buffer! Error code: %s", string_VkResult(result));

		// Get the staging buffer's memory requirements
		VkMemoryRequirements stagingMemRequirements;
		vkGetBufferMemoryRequirements(device->GetDevice(), stagingBuffer, &stagingMemRequirements);

		// Get the staging buffer's memory type index
		uint32_t stagingMemTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemRequirements.memoryTypeBits);
		if(stagingMemTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan particle staging buffer!");
		
		// Set the memory alloc info
		VkMemoryAllocateInfo stagingAllocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = stagingMemRequirements.size,
			.memoryTypeIndex = stagingMemTypeIndex
		};

		// Allocate the staging buffer's memory
		VkDeviceMemory stagingMemory;
		result = vkAllocateMemory(device->GetDevice(), &stagingAllocInfo, nullptr, &stagingMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle staging buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the staging buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), stagingBuffer, stagingMemory, 0);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan particle staging buffer to its memory! Error code: %s", string_VkResult(result));
		
		// Map the staging buffer's memory
		void* stagingData;
		result = vkMapMemory(device->GetDevice(), stagingMemory, 0, VK_WHOLE_SIZE, 0, &stagingData);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to map Vulkan particle staging buffer memory! Error code: %s", string_VkResult(result));
		
		// Copy the particle infos to the staging buffer
		memcpy(stagingData, particles, alignedParticleCount * sizeof(Particle));

		// Unmap the staging buffer's memory
		vkUnmapMemory(device->GetDevice(), stagingMemory);

		// Set the particle buffer create info
		VkBufferCreateInfo bufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = alignedParticleCount * sizeof(Particle),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = (device->GetQueueFamilyIndexArraySize() == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = device->GetQueueFamilyIndexArraySize(),
			.pQueueFamilyIndices = device->GetQueueFamilyIndexArray()
		};

		// Create the particle buffers
		for(uint32_t i = 0; i != 3; ++i) {
			result = vkCreateBuffer(device->GetDevice(), &bufferInfo, nullptr, buffers + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffer! Error code: %s", string_VkResult(result));
		}

		// Get the buffers' memory requirements
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device->GetDevice(), buffers[0], &memRequirements);

		// Get the buffers' memory type index
		uint32_t memTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memRequirements.memoryTypeBits);
		if(memTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan particle buffer!");
		
		// Align the required size to the required alignment
		VkDeviceSize alignedSize = (memRequirements.size + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);
		
		// Set the memory alloc info
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = alignedSize * 3,
			.memoryTypeIndex = memTypeIndex
		};

		// Allocare the buffers' memory
		result = vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &bufferMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the buffers to their memory
		for(uint32_t i = 0; i != 3; ++i) {
			result = vkBindBufferMemory(device->GetDevice(), buffers[i], bufferMemory, i * alignedSize);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan particle buffers to their memory! Error code: %s", string_VkResult(result));
		}

		// Set the transfer command buffer alloc info
		VkCommandBufferAllocateInfo commandBufferAllocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = device->GetTransferCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		// Allocate the transfer command buffer
		VkCommandBuffer commandBuffer;
		result = vkAllocateCommandBuffers(device->GetDevice(), &commandBufferAllocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Set the command buffer begin info
		VkCommandBufferBeginInfo commandBufferBeginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Transfer all particles from the staging buffer to all particle buffers
		VkBufferCopy copyRegion {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(Particle)
		};

		for(uint32_t i = 0; i != 3; ++i)
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffers[i], 1, &copyRegion);
		
		// End recording the command buffer
		vkEndCommandBuffer(commandBuffer);

		// Set the transfer fence create info
		VkFenceCreateInfo transferFenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// Create the transfer fence
		VkFence transferFence;
		result = vkCreateFence(device->GetDevice(), &transferFenceInfo, nullptr, &transferFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle transfer fence! Error code: %s", string_VkResult(result));
		
		// Set the submit info
		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		// Submit the command buffer
		result = vkQueueSubmit(device->GetTransferQueue(), 1, &submitInfo, transferFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to submit the Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Wait for the command buffer to finish execution
		result = vkWaitForFences(device->GetDevice(), 1, &transferFence, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to wait for Vulkan particle transfer command completion! Error code: %s", string_VkResult(result));
		
		// Destroy all objects used for the transfer operation
		vkFreeCommandBuffers(device->GetDevice(), device->GetTransferCommandPool(), 1, &commandBuffer);
		vkFreeMemory(device->GetDevice(), stagingMemory, nullptr);
		vkDestroyFence(device->GetDevice(), transferFence, nullptr);
		vkDestroyBuffer(device->GetDevice(), stagingBuffer, nullptr);
	}

	// Public functions
	ParticleSystem::ParticleSystem(VulkanDevice* device, const char* filePath, float systemSize, float gravitationalConst, float simulationTime, float softeningLen, size_t particleCountAlignment) : device(device), systemSize(systemSize), particleCount(0), gravitationalConst(gravitationalConst), simulationTime(simulationTime), softeningLen(softeningLen) {
		// Open the given file
		FILE* fileInput = fopen(filePath, "r");
		if(!fileInput)
			GSIM_THROW_EXCEPTION("Failed to open particle input file!");
		
		// Allocate a dynamic particle array
		size_t particleCapacity = particleCountAlignment;
		Particle* particles = (Particle*)malloc(sizeof(Particle) * particleCapacity);
		if(!particles)
			GSIM_THROW_EXCEPTION("Failed to allocate particle array!");
		
		// Load particles from the given file until none are left
		Particle particle;
		while(fscanf(fileInput, "%f%f%f%f%f", &particle.pos.x, &particle.pos.y, &particle.vel.x, &particle.vel.y, &particle.mass) == 5) {
			// Check if there is room in the array for the new particle
			++particleCount;
			if(particleCount > particleCapacity) {
				// Double the array's capacity
				particleCapacity <<= 1;

				// Reallocate the array
				particles = (Particle*)realloc(particles, particleCapacity * sizeof(Particle));
				if(!particles)
					GSIM_THROW_EXCEPTION("Failed to reallocate particle array!");
			}

			// Add the new particle to the array
			particles[particleCount - 1] = particle;
		}

		// Close the file
		fclose(fileInput);

		// Throw an exception if no particle was read from the file
		if(!particleCount)
			GSIM_THROW_EXCEPTION("The particle input file must contain at least one valid particle!");
		
		// Set the aligned particle count
		alignedParticleCount = (particleCount + particleCountAlignment - 1) & ~(particleCountAlignment - 1);

		// Create the Vulkan objects
		CreateVulkanObjects(particles);

		// Free the particles array
		free(particles);
	}
	ParticleSystem::ParticleSystem(VulkanDevice* device, size_t particleCount, GenerateType generateType, float generateSize, float minMass, float maxMass, float systemSize, float gravitationalConst, float simulationTime, float softeningLen, size_t particleCountAlignment) : device(device), particleCount(particleCount), alignedParticleCount((particleCount + particleCountAlignment - 1) & ~(particleCountAlignment - 1)), systemSize(systemSize), gravitationalConst(gravitationalConst), simulationTime(simulationTime), softeningLen(softeningLen) {
		// Round the particle count down to the nearest even integer if the generate type is set to GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION
		if(generateType == GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION) {
			particleCount &= ~1;
			this->particleCount &= ~1;
		}

		// Throw an exception if there are no particles in the system
		if(!particleCount)
			GSIM_THROW_EXCEPTION("The simulation must contain at least one particle!");
		
		// Allocate the particle array
		Particle* particles = (Particle*)malloc(alignedParticleCount * sizeof(Particle));
		if(!particles)
			GSIM_THROW_EXCEPTION("Failed to allocate particle array!");
		
		// Generate the particles based on the generate type
		switch(generateType) {
		case GENERATE_TYPE_RANDOM:
			GenerateParticlesRandom(particles, generateSize, minMass, maxMass);
			break;
		case GENERATE_TYPE_GALAXY:
			GenerateParticlesGalaxy(particles, generateSize, minMass, maxMass);
			break;
		case GENERATE_TYPE_GALAXY_COLLISION:
			GenerateParticlesGalaxyCollision(particles, generateSize, minMass, maxMass);
			break;
		case GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION:
			GenerateParticlesSymmetricalGalaxyCollision(particles, generateSize, minMass, maxMass);
			break;
		}

		// Fill the remaining particle infos
		for(size_t i = particleCount; i != alignedParticleCount; ++i)
			particles[i] = { 0, 0, 0, 0, 0 };

		// Create the Vulkan objects
		CreateVulkanObjects(particles);

		// Free the particles array
		free(particles);
	}
	
	void ParticleSystem::GetParticles(Particle* particles) {
		// Set the staging buffer create info
		uint32_t transferIndex = device->GetQueueFamilyIndices().transferIndex;

		VkBufferCreateInfo stagingBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = (VkDeviceSize)(alignedParticleCount * sizeof(Particle)),
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &transferIndex
		};

		// Create the staging buffer
		VkBuffer stagingBuffer;
		VkResult result = vkCreateBuffer(device->GetDevice(), &stagingBufferInfo, nullptr, &stagingBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle staging buffer! Error code: %s", string_VkResult(result));

		// Get the staging buffer's memory requirements
		VkMemoryRequirements stagingMemRequirements;
		vkGetBufferMemoryRequirements(device->GetDevice(), stagingBuffer, &stagingMemRequirements);

		// Get the staging buffer's memory type index
		uint32_t stagingMemTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemRequirements.memoryTypeBits);
		if(stagingMemTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan particle staging buffer!");
		
		// Set the memory alloc info
		VkMemoryAllocateInfo stagingAllocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = stagingMemRequirements.size,
			.memoryTypeIndex = stagingMemTypeIndex
		};

		// Allocate the staging buffer's memory
		VkDeviceMemory stagingMemory;
		result = vkAllocateMemory(device->GetDevice(), &stagingAllocInfo, nullptr, &stagingMemory);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle staging buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the staging buffer to its memory
		result = vkBindBufferMemory(device->GetDevice(), stagingBuffer, stagingMemory, 0);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to bind Vulkan particle staging buffer to its memory! Error code: %s", string_VkResult(result));
		
		// Set the transfer command buffer alloc info
		VkCommandBufferAllocateInfo commandBufferAllocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = device->GetTransferCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		// Allocate the transfer command buffer
		VkCommandBuffer commandBuffer;
		result = vkAllocateCommandBuffers(device->GetDevice(), &commandBufferAllocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Set the command buffer begin info
		VkCommandBufferBeginInfo commandBufferBeginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Transfer the latest particle infos to the staging buffer
		VkBufferCopy copyRegion {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(Particle)
		};

		vkCmdCopyBuffer(commandBuffer, buffers[computeInputIndex], stagingBuffer, 1, &copyRegion);
		
		// End recording the command buffer
		vkEndCommandBuffer(commandBuffer);

		// Set the transfer fence create info
		VkFenceCreateInfo transferFenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// Create the transfer fence
		VkFence transferFence;
		result = vkCreateFence(device->GetDevice(), &transferFenceInfo, nullptr, &transferFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan particle transfer fence! Error code: %s", string_VkResult(result));
		
		// Set the submit info
		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		// Submit the command buffer
		result = vkQueueSubmit(device->GetTransferQueue(), 1, &submitInfo, transferFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to submit the Vulkan particle transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Wait for the command buffer to finish execution
		result = vkWaitForFences(device->GetDevice(), 1, &transferFence, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to wait for Vulkan particle transfer command completion! Error code: %s", string_VkResult(result));

		// Destroy all objects used for the transfer operation
		vkFreeCommandBuffers(device->GetDevice(), device->GetTransferCommandPool(), 1, &commandBuffer);
		vkDestroyFence(device->GetDevice(), transferFence, nullptr);

		// Map the staging buffer's memory
		void* stagingData;
		result = vkMapMemory(device->GetDevice(), stagingMemory, 0, VK_WHOLE_SIZE, 0, &stagingData);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to map Vulkan particle staging buffer memory! Error code: %s", string_VkResult(result));

		// Copy the staging buffer's data to the given particle array
		memcpy(particles, stagingData, particleCount * sizeof(Particle));

		// Unmap the staging buffer's memory
		vkUnmapMemory(device->GetDevice(), stagingMemory);

		// Destroy the staging buffer
		vkFreeMemory(device->GetDevice(), stagingMemory, nullptr);
		vkDestroyBuffer(device->GetDevice(), stagingBuffer, nullptr);
	}
	void ParticleSystem::SaveParticles(const char* filePath) {
		// Open the file stream
		FILE* fileOutput = fopen(filePath, "w");
		if(!fileOutput)
			GSIM_THROW_EXCEPTION("Failed to open particle output file!");

		// Allocate the particle array
		Particle* particles = (Particle*)malloc(particleCount * sizeof(Particle));
		if(!particles)
			GSIM_THROW_EXCEPTION("Failed to allocate particle array!");
		
		// Get all particles
		GetParticles(particles);

		// Save the particles to the file stream
		for(size_t i = 0; i != particleCount; ++i)
			fprintf(fileOutput, "%.7f %.7f %.7f %.7f %.7f\n", particles[i].pos.x, particles[i].pos.y, particles[i].vel.x, particles[i].vel.y, particles[i].mass);
		
		// Close the file stream
		fclose(fileOutput);

		// Free the particle array
		free(particles);
	}

	ParticleSystem::~ParticleSystem() {
		// Destroy all Vulkan objects
		vkFreeMemory(device->GetDevice(), bufferMemory, nullptr);
		for(uint32_t i = 0; i != 3; ++i)
			vkDestroyBuffer(device->GetDevice(), buffers[i], nullptr);
	}
}