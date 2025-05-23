#include "ParticleSystem.hpp"
#include "Debug/Exception.hpp"
#include "Simulation/BarnesHut/BarnesHutSimulation.hpp"
#include "Simulation/Direct/DirectSimulation.hpp"
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <random>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Internal helper functions
	void ParticleSystem::GenerateParticlesRandom(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Create the random engine and distribution
		std::default_random_engine randomEngine;
		randomEngine.seed((uint32_t)time(nullptr));
		std::uniform_real_distribution<float> distribution(0, 1);

		// Generate every particle
		for(size_t i = 0; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar
			float theta = (float)(distribution(randomEngine) * 2 * M_PI);
			float r = sqrt(1 - distribution(randomEngine)) * generateSize;

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * cosf(theta), r * sinf(theta) };

			// Set the particle's velocity to zero
			particles[i].vel = { 0, 0 };

			// Generate the particle's mass
			particles[i].mass = distribution(randomEngine) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesGalaxy(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Create the random engine and distribution
		std::default_random_engine randomEngine;
		randomEngine.seed((uint32_t)time(nullptr));
		std::uniform_real_distribution<float> distribution(0, 1);

		// Calculate the orbit velocity
		float avgMass = (minMass + maxMass) * 0.5f;
		float vel = sqrt(gravitationalConst * particleCount * avgMass / (generateSize * generateSize * generateSize));

		// Generate every particle
		for(size_t i = 0; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar
			float theta = (float)(distribution(randomEngine) * 2 * M_PI);
			float r = (1 - distribution(randomEngine)) * generateSize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * vel, thetaCos * r * vel };

			// Generate the particle's mass
			particles[i].mass = distribution(randomEngine) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Create the random engine and distribution
		std::default_random_engine randomEngine;
		randomEngine.seed((uint32_t)time(nullptr));
		std::uniform_real_distribution<float> distribution(0, 1);

		// Calculate the orbit velocity
		float galaxySize = generateSize / 3;
		float avgMass = (minMass + maxMass) * 0.25f;
		float vel = sqrt(gravitationalConst * particleCount * avgMass / (galaxySize * galaxySize * galaxySize));

		// Generate every particle in the first galaxy
		for(size_t i = 0; i != particleCount >> 1; ++i) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = (float)(distribution(randomEngine) * 2 * M_PI);
			float r = (1 - distribution(randomEngine)) * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos - galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * vel + galaxySize * 0.1f, thetaCos * r * vel };

			// Generate the particle's mass
			particles[i].mass = distribution(randomEngine) * (maxMass - minMass) + minMass;
		}

		// Generate avery particle in the second galaxy
		for(size_t i = particleCount >> 1; i != particleCount; ++i) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = (float)(distribution(randomEngine) * 2 * M_PI);
			float r = (1 - distribution(randomEngine)) * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos + galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * vel - galaxySize * 0.1f, thetaCos * r * vel };

			// Generate the particle's mass
			particles[i].mass = distribution(randomEngine) * (maxMass - minMass) + minMass;
		}
	}
	void ParticleSystem::GenerateParticlesSymmetricalGalaxyCollision(Particle* particles, float generateSize, float minMass, float maxMass) {
		// Create the random engine and distribution
		std::default_random_engine randomEngine;
		randomEngine.seed((uint32_t)time(nullptr));
		std::uniform_real_distribution<float> distribution(0, 1);

		// Calculate the orbit velocity
		float galaxySize = generateSize / 3;
		float avgMass = (minMass + maxMass) * 0.25f;
		float vel = sqrt(gravitationalConst * particleCount * avgMass / (galaxySize * galaxySize * galaxySize));

		// Generate every particle in the first galaxy and mirror it in the second
		for(size_t i = 0; i != particleCount; i += 2) {
			// Generate the particle's coordinates as polar relative to the galaxy's center
			float theta = (float)(distribution(randomEngine) * 2 * M_PI);
			float r = (1 - distribution(randomEngine)) * galaxySize;

			float thetaSin = sinf(theta);
			float thetaCos = cosf(theta);

			// Convert the polar coordinates to cartesian
			particles[i].pos = { r * thetaCos - galaxySize * 2, r * thetaSin };

			// Set the particle's velocity to slowly orbit the galaxy's center
			particles[i].vel = { -thetaSin * r * vel + galaxySize * 0.1f, thetaCos * r * vel };

			// Generate the particle's mass
			particles[i].mass = distribution(randomEngine) * (maxMass - minMass) + minMass;

			// Mirror the particle from the first galaxy in the second
			particles[i + 1].pos = { -particles[i].pos.x, particles[i].pos.y };
			particles[i + 1].vel = { -thetaSin * r * vel - galaxySize * 0.1f, -thetaCos * r * vel };
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
			.size = alignedParticleCount * ((sizeof(Vec2) << 1) + sizeof(float)),
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
		Vec2* vec2Iter = (Vec2*)stagingData;

		for(size_t i = 0; i != alignedParticleCount; ++i, ++vec2Iter)
			*vec2Iter = particles[i].pos;

		for(size_t i = 0; i != alignedParticleCount; ++i, ++vec2Iter)
			*vec2Iter = particles[i].vel;

		float* floatIter = (float*)vec2Iter;

		for(size_t i = 0; i != alignedParticleCount; ++i, ++floatIter)
			*floatIter = particles[i].mass;

		// Unmap the staging buffer's memory
		vkUnmapMemory(device->GetDevice(), stagingMemory);

		// Set the particle position and velocity buffer create info
		VkBufferCreateInfo posVelBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = alignedParticleCount * sizeof(Vec2),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = (device->GetQueueFamilyIndexArraySize() == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = device->GetQueueFamilyIndexArraySize(),
			.pQueueFamilyIndices = device->GetQueueFamilyIndexArray()
		};

		// Set the particle mass buffer create info
		VkBufferCreateInfo massBufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = alignedParticleCount * sizeof(float),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = (device->GetQueueFamilyIndexArraySize() == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = device->GetQueueFamilyIndexArraySize(),
			.pQueueFamilyIndices = device->GetQueueFamilyIndexArray()
		};

		// Create the particle buffers
		for(uint32_t i = 0; i != 3; ++i) {
			// Create the position buffer
			result = vkCreateBuffer(device->GetDevice(), &posVelBufferInfo, nullptr, &(buffers[i].posBuffer));
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffers! Error code: %s", string_VkResult(result));
			
			// Create the velocity buffer
			result = vkCreateBuffer(device->GetDevice(), &posVelBufferInfo, nullptr, &(buffers[i].velBuffer));
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffers! Error code: %s", string_VkResult(result));

			// Create the mass buffer
			result = vkCreateBuffer(device->GetDevice(), &massBufferInfo, nullptr, &(buffers[i].massBuffer));
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan particle buffers! Error code: %s", string_VkResult(result));
		}

		// Get the buffers' memory requirements
		VkMemoryRequirements posVelMemRequirements, massMemRequirements;
		vkGetBufferMemoryRequirements(device->GetDevice(), buffers[0].posBuffer, &posVelMemRequirements);
		vkGetBufferMemoryRequirements(device->GetDevice(), buffers[0].massBuffer, &massMemRequirements);

		// Get the buffers' memory type index
		uint32_t memTypeIndex = device->GetMemoryTypeIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, posVelMemRequirements.memoryTypeBits & massMemRequirements.memoryTypeBits);
		if(memTypeIndex == UINT32_MAX)
			GSIM_THROW_EXCEPTION("Failed to find supported memory type for Vulkan particle buffer!");
		
		// Align the required sizes to the required alignment
		VkDeviceSize maxAlignment = (posVelMemRequirements.alignment > massMemRequirements.alignment) ? posVelMemRequirements.alignment : massMemRequirements.alignment;
		VkDeviceSize alignedPosVelSize = (posVelMemRequirements.size + maxAlignment - 1) & ~(maxAlignment - 1);
		VkDeviceSize alignedMassSize = (massMemRequirements.size + maxAlignment - 1) & ~(maxAlignment - 1);
		VkDeviceSize alignedSize = (alignedPosVelSize << 1) + alignedMassSize;
		
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
			// Bind the position buffer
			result = vkBindBufferMemory(device->GetDevice(), buffers[i].posBuffer, bufferMemory, i * alignedSize);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan particle buffers to their memory! Error code: %s", string_VkResult(result));
			
			// Bind the velocity buffer
			result = vkBindBufferMemory(device->GetDevice(), buffers[i].velBuffer, bufferMemory, i * alignedSize + alignedPosVelSize);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to bind Vulkan particle buffers to their memory! Error code: %s", string_VkResult(result));
			
			// Bind the mass buffer
			result = vkBindBufferMemory(device->GetDevice(), buffers[i].massBuffer, bufferMemory, i * alignedSize + (alignedPosVelSize << 1));
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
		
		// Set the copy regions for all particle components
		VkBufferCopy posCopyRegion {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(Vec2)
		};
		VkBufferCopy velCopyRegion {
			.srcOffset = alignedParticleCount * sizeof(Vec2),
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(Vec2)
		};
		VkBufferCopy massCopyRegion {
			.srcOffset = alignedParticleCount * (sizeof(Vec2) << 1),
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(float)
		};

		// Transfer all particles from the staging buffer to all particle buffers
		for(uint32_t i = 0; i != 3; ++i) {
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffers[i].posBuffer, 1, &posCopyRegion);
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffers[i].velBuffer, 1, &velCopyRegion);
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffers[i].massBuffer, 1, &massCopyRegion);
		}
		
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
	void ParticleSystem::GetCameraInfo(const Particle* particles) {
		Vec2 minCoords { INFINITY, INFINITY };
		Vec2 maxCoords { -INFINITY, -INFINITY };

		for(uint32_t i = 0; i != particleCount; ++i) {
			Vec2 pos = particles[i].pos;

			if(pos.x < minCoords.x)
				minCoords.x = pos.x;
			if(pos.x > maxCoords.x)
				maxCoords.x = pos.x;
			if(pos.y < minCoords.y)
				minCoords.y = pos.y;
			if(pos.y > maxCoords.y)
				maxCoords.y = pos.y;
		}

		cameraStartPos = { (minCoords.x + maxCoords.x) * 0.5f, (minCoords.y + maxCoords.y) * 0.5f };
		
		float width = maxCoords.x - minCoords.x;
		float height = maxCoords.y - minCoords.y;

		if(width > height) {
			cameraStartSize = width;
		} else {
			cameraStartSize = height;
		}
	}

	// Public functions
	ParticleSystem::ParticleSystem(VulkanDevice* device, const char* filePath, float gravitationalConst, float simulationTime, float simulationSpeed, float softeningLen, float accuracyParameter, SimulationAlgorithm simulationAlgorithm) : device(device), particleCount(0), gravitationalConst(gravitationalConst), simulationTime(simulationTime), simulationSpeed(simulationSpeed), accuracyParameter(accuracyParameter), softeningLen(softeningLen) {
		// Get the particle count alignment
		size_t particleCountAlignment;
		if(simulationAlgorithm == SIMULATION_ALGORITHM_DIRECT_SUM) {
			particleCountAlignment = DirectSimulation::GetRequiredParticleAlignment();
		} else if(simulationAlgorithm == SIMULATION_ALGORITHM_BARNES_HUT) {
			particleCountAlignment = BarnesHutSimulation::GetRequiredParticleAlignment();
		} else {
			GSIM_THROW_EXCEPTION("Invalid simulation algorithm requested!");
		}

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

		// Fill the remaining particle infos
		for(size_t i = particleCount; i != alignedParticleCount; ++i)
			particles[i] = { 0, 0, 0, 0, 0 };

		// Create the Vulkan objects
		CreateVulkanObjects(particles);

		// Get the camera's starting info
		GetCameraInfo(particles);

		// Free the particles array
		free(particles);
	}
	ParticleSystem::ParticleSystem(VulkanDevice* device, size_t particleCount, GenerateType generateType, float generateSize, float minMass, float maxMass, float gravitationalConst, float simulationTime, float simulationSpeed, float softeningLen, float accuracyParameter, SimulationAlgorithm simulationAlgorithm) : device(device), particleCount(particleCount), gravitationalConst(gravitationalConst), simulationTime(simulationTime), simulationSpeed(simulationSpeed), accuracyParameter(accuracyParameter), softeningLen(softeningLen) {
		// Get the particle count alignment
		size_t particleCountAlignment;
		if(simulationAlgorithm == SIMULATION_ALGORITHM_DIRECT_SUM) {
			particleCountAlignment = DirectSimulation::GetRequiredParticleAlignment();
		} else if(simulationAlgorithm == SIMULATION_ALGORITHM_BARNES_HUT) {
			particleCountAlignment = BarnesHutSimulation::GetRequiredParticleAlignment();
		} else {
			GSIM_THROW_EXCEPTION("Invalid simulation algorithm requested!");
		}

		// Round the particle count down to the nearest even integer if the generate type is set to GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION
		if(generateType == GENERATE_TYPE_SYMMETRICAL_GALAXY_COLLISION) {
			particleCount &= ~1;
			this->particleCount &= ~1;
		}

		// Throw an exception if there are no particles in the system
		if(!particleCount)
			GSIM_THROW_EXCEPTION("The simulation must contain at least one particle!");

		// Set the aligned particle count
		alignedParticleCount = (particleCount + particleCountAlignment - 1) & ~(particleCountAlignment - 1);

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

		// Get the camera's starting info
		GetCameraInfo(particles);

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
			.size = alignedParticleCount * ((sizeof(Vec2) << 1) + sizeof(float)),
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
		
		// Set the copy regions for all particle components
		VkBufferCopy posCopyRegion {
			.srcOffset = 0,
			.dstOffset = 0,
			.size = alignedParticleCount * sizeof(Vec2)
		};
		VkBufferCopy velCopyRegion {
			.srcOffset = 0,
			.dstOffset = alignedParticleCount * sizeof(Vec2),
			.size = alignedParticleCount * sizeof(Vec2)
		};
		VkBufferCopy massCopyRegion {
			.srcOffset = 0,
			.dstOffset = alignedParticleCount * (sizeof(Vec2) << 1),
			.size = alignedParticleCount * sizeof(float)
		};

		// Transfer the latest particle infos to the staging buffer
		vkCmdCopyBuffer(commandBuffer, buffers[computeInputIndex].posBuffer, stagingBuffer, 1, &posCopyRegion);
		vkCmdCopyBuffer(commandBuffer, buffers[computeInputIndex].velBuffer, stagingBuffer, 1, &velCopyRegion);
		vkCmdCopyBuffer(commandBuffer, buffers[computeInputIndex].massBuffer, stagingBuffer, 1, &massCopyRegion);
		
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
		Vec2* vec2Iter = (Vec2*)stagingData;
		for(size_t i = 0; i != particleCount; ++i)
			particles[i].pos = vec2Iter[i];
		vec2Iter += alignedParticleCount;
		for(size_t i = 0; i != particleCount; ++i)
			particles[i].vel = vec2Iter[i];
		float* floatIter = (float*)(vec2Iter + alignedParticleCount);
		for(size_t i = 0; i != particleCount; ++i)
			particles[i].mass = floatIter[i];

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
		for(uint32_t i = 0; i != 3; ++i) {
			vkDestroyBuffer(device->GetDevice(), buffers[i].posBuffer, nullptr);
			vkDestroyBuffer(device->GetDevice(), buffers[i].velBuffer, nullptr);
			vkDestroyBuffer(device->GetDevice(), buffers[i].massBuffer, nullptr);
		}
	}
}