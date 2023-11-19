#include "Points.hpp"
#include "Debug/Logger.hpp"
#include "Manager/Loader.hpp"
#include "Manager/Parser.hpp"
#include "Utils/Point.hpp"
#include "Vulkan/Core/Allocator.hpp"
#include "Vulkan/Core/CommandPool.hpp"
#include "Vulkan/Core/Device.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

namespace gsim {
	// Variables
	static size_t pointCount;

	static VkBuffer pointBuffers[POINT_BUFFER_COUNT];
	static VkDeviceSize pointBufferSize;
	static VkDeviceMemory pointBuffersMemory;

	static uint32_t computeIndices[POINT_BUFFER_COUNT - 1];
	static uint32_t graphicsIndex;

	static Vector2 screenPos;
	static Vector2 screenMinSize;

	// Public functions
	void CreatePointBuffers() {
		// Load the point count
		if(GetPointInFileName())
			pointCount = LoadPoints(GetPointInFileName(), nullptr);
		else
			pointCount = GetArgsPointCount();
		pointBufferSize = (VkDeviceSize)(sizeof(Point) * pointCount);

		// Load the device's queue family indices
		VulkanQueueFamilyIndices queueFamilyIndices = GetVulkanDeviceQueueFamilyIndices();

		// Set the staging buffer create info
		VkBufferCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.size = pointBufferSize;
		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = &queueFamilyIndices.transferQueueIndex;

		// Create the staging buffer
		VkBuffer stagingBuffer;

		VkResult result = vkCreateBuffer(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &stagingBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan point staging buffer! Error code: %s", string_VkResult(result));
		
		// Get the staging buffer's memory requirements
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(GetVulkanDevice(), stagingBuffer, &memoryRequirements);

		// Set the staging buffer's alloc info
		VkMemoryAllocateInfo allocInfo;

		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindVulkanMemoryType(0, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		// Allocate the staging buffer's memory
		VkDeviceMemory stagingBufferMemory;

		result = vkAllocateMemory(GetVulkanDevice(), &allocInfo, GetVulkanAllocCallbacks(), &stagingBufferMemory);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan point staging buffer memory! Error code: %s", string_VkResult(result));
		
		// Bind the staging buffer to its memory
		result = vkBindBufferMemory(GetVulkanDevice(), stagingBuffer, stagingBufferMemory, 0);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to bind Vulkan point staging buffer memory! Error code: %s", string_VkResult(result));
		
		// Read every point into the staging buffer
		void* stagingBufferData;
		result = vkMapMemory(GetVulkanDevice(), stagingBufferMemory, 0, pointBufferSize, 0, &stagingBufferData);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to map Vulkan point staging buffer memory! Error code: %s", string_VkResult(result));

		if(GetPointInFileName()) {
			// Load all points from the file
			LoadPoints(GetPointInFileName(), (Point*)stagingBufferData);

			// Loop through every point and set the screen's coords
			Point* points = (Point*)stagingBufferData;
			screenPos = { 0.0, 0.0 };

			for(size_t i = 0; i != pointCount; ++i) {
				screenPos.x += points[i].pos.x;
				screenPos.y += points[i].pos.y;
			}

			screenPos.x /= pointCount;
			screenPos.y /= pointCount;

			// Set the screen's min size
			screenMinSize = { 0.0, 0.0 };

			for(size_t i = 0; i != pointCount; ++i) {
				// Calculate the current point's relative position
				Vector2 relPos{ points[i].pos.x - screenPos.x, points[i].pos.y - screenPos.y };

				// Make the relative position's coordinates absolute
				relPos.x = fabs(relPos.x);
				relPos.y = fabs(relPos.y);

				// Check if the screen's min size needs to be changed
				if(relPos.x > screenMinSize.x)
					screenMinSize.x = relPos.x;
				if(relPos.y > screenMinSize.y)
					screenMinSize.y = relPos.y;
			}

			screenMinSize.x *= 2.0;
			screenMinSize.y *= 2.0;
			screenMinSize.x += 2.0;
			screenMinSize.y += 2.0;
		} else {
			// Set the random seed
			srand(time(nullptr));

			// Calculate the angle to apply for every new point
			double pointAngle = 2.0 * M_PI / pointCount;

			// Loop through every point
			Point* points = (Point*)stagingBufferData;
			double angle = 0.0;

			for(size_t i = 0; i != pointCount; ++i) {
				// Calculate the current point's normalized position
				Vector2 pos{ cos(angle), sin(angle) };

				// Set the point's position and velocity
				points[i].pos = { pos.x * 100.0, pos.y * 100.0 };
				points[i].vel = { -pos.x * 5.0, -pos.y * 5.0 };

				// Generate a random mass
				double mass = ((double)rand() / (double)RAND_MAX) * 99.0 + 1.0;

				// Set the point's mass
				points[i].mass = mass;

				// Increase the angle
				angle += pointAngle;
			}

			// Set the screen position and min size
			screenPos = { 0.0, 0.0 };
			screenMinSize = { 202.0, 202.0 };
		}

		// Flush the changes and unmap the memory
		VkMappedMemoryRange stagingBufferMemoryRange;

		stagingBufferMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		stagingBufferMemoryRange.pNext = nullptr;
		stagingBufferMemoryRange.memory = stagingBufferMemory;
		stagingBufferMemoryRange.offset = 0;
		stagingBufferMemoryRange.size = pointBufferSize;

		result = vkFlushMappedMemoryRanges(GetVulkanDevice(), 1, &stagingBufferMemoryRange);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to flush Vulkan point staging buffer memory! Error code: %s", string_VkResult(result));

		vkUnmapMemory(GetVulkanDevice(), stagingBufferMemory);

		// Set the point buffer create info
		createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		uint32_t bufferQueueFamilyIndices[3];
		if(queueFamilyIndices.graphicsQueueIndex != queueFamilyIndices.transferQueueIndex && queueFamilyIndices.transferQueueIndex != queueFamilyIndices.computeQueueIndex) {
			// Set the buffer queue family indices
			bufferQueueFamilyIndices[0] = queueFamilyIndices.graphicsQueueIndex;
			bufferQueueFamilyIndices[1] = queueFamilyIndices.transferQueueIndex;
			bufferQueueFamilyIndices[2] = queueFamilyIndices.computeQueueIndex;

			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 3;
			createInfo.pQueueFamilyIndices = bufferQueueFamilyIndices;
		} else if(queueFamilyIndices.graphicsQueueIndex == queueFamilyIndices.transferQueueIndex) {
			// Set the buffer queue family indices
			bufferQueueFamilyIndices[0] = queueFamilyIndices.graphicsQueueIndex;
			bufferQueueFamilyIndices[1] = queueFamilyIndices.computeQueueIndex;

			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = bufferQueueFamilyIndices;
		} else if(queueFamilyIndices.graphicsQueueIndex == queueFamilyIndices.computeQueueIndex) {
			// Set the buffer queue family indices
			bufferQueueFamilyIndices[0] = queueFamilyIndices.graphicsQueueIndex;
			bufferQueueFamilyIndices[1] = queueFamilyIndices.transferQueueIndex;

			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = bufferQueueFamilyIndices;
		} else if(queueFamilyIndices.transferQueueIndex == queueFamilyIndices.computeQueueIndex) {
			// Set the buffer queue family indices
			bufferQueueFamilyIndices[0] = queueFamilyIndices.graphicsQueueIndex;
			bufferQueueFamilyIndices[1] = queueFamilyIndices.transferQueueIndex;

			createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = bufferQueueFamilyIndices;
		} else {
			// Set the buffer queue family indices
			bufferQueueFamilyIndices[0] = queueFamilyIndices.graphicsQueueIndex;

			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 1;
			createInfo.pQueueFamilyIndices = bufferQueueFamilyIndices;
		}

		// Create the point buffers
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			result = vkCreateBuffer(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), pointBuffers + i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan point buffer! Error code: %s", string_VkResult(result));
		}

		// Get the point buffer memory requirements
		vkGetBufferMemoryRequirements(GetVulkanDevice(), pointBuffers[0], &memoryRequirements);

		// Set the point buffers' alloc info
		VkDeviceSize alignedSize = (memoryRequirements.size + memoryRequirements.alignment - 1) & ~(memoryRequirements.alignment - 1);

		allocInfo.allocationSize = alignedSize * POINT_BUFFER_COUNT;
		allocInfo.memoryTypeIndex = FindVulkanMemoryType(0, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		// Allocate the point buffers' memory
		result = vkAllocateMemory(GetVulkanDevice(), &allocInfo, GetVulkanAllocCallbacks(), &pointBuffersMemory);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan point buffers memory! Error code: %s", string_VkResult(result));
		
		// Bind the point buffers to their memory
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			result = vkBindBufferMemory(GetVulkanDevice(), pointBuffers[i], pointBuffersMemory, alignedSize * i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to bind Vulkan point buffer memory! Error code: %s", string_VkResult(result));
		}

		// Set the transfer command buffer alloc info
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo commandBufferAllocInfo;

		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.pNext = nullptr;
		commandBufferAllocInfo.commandPool = GetVulkanTransferCommandPool();
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = 1;

		// Allocate the transfer command buffer
		result = vkAllocateCommandBuffers(GetVulkanDevice(), &commandBufferAllocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan point buffer transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Set the transfer command buffer begin info
		VkCommandBufferBeginInfo commandBufferBeginInfo;

		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		// Begin recording the transfer command buffer
		result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to begin recording Vulkan point buffer transfer command buffer! Error code: %s", string_VkResult(result));
		
		// Set the copy region
		VkBufferCopy copyRegion;

		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = pointBufferSize;

		// Record every copy command to the command buffer
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkCmdCopyBuffer(commandBuffer, stagingBuffer, pointBuffers[i], 1, &copyRegion);
		
		// End recording the transfer command buffer
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

		// Submit the transfer command buffer and wait for it to finish execution
		result = vkQueueSubmit(GetVulkanTransferQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to submit Vulkan point buffer transfer command buffer! Error code: %s", string_VkResult(result));

		result = vkQueueWaitIdle(GetVulkanTransferQueue());
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for Vulkan transfer queue idle! Error code: %s", string_VkResult(result));
		
		// Set the buffer indices
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT - 1; ++i)
			computeIndices[i] = i;
		graphicsIndex = POINT_BUFFER_COUNT - 1;

		// Free the command buffer
		vkFreeCommandBuffers(GetVulkanDevice(), GetVulkanTransferCommandPool(), 1, &commandBuffer);

		// Destroy the staging buffer and free its memory
		vkDestroyBuffer(GetVulkanDevice(), stagingBuffer, GetVulkanAllocCallbacks());
		vkFreeMemory(GetVulkanDevice(), stagingBufferMemory, GetVulkanAllocCallbacks());

		GSIM_LOG_INFO("Created Vulkan point buffers.");
	}
	void DestroyPointBuffers() {
		// Destroy every point command buffer and free their memory
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroyBuffer(GetVulkanDevice(), pointBuffers[i], GetVulkanAllocCallbacks());
		vkFreeMemory(GetVulkanDevice(), pointBuffersMemory, GetVulkanAllocCallbacks());
	}

	size_t GetPointCount() {
		return pointCount;
	}
	VkBuffer* GetPointBuffers() {
		return pointBuffers;
	}

	Vector2 GetScreenPos() {
		return screenPos;
	}
	Vector2 GetScreenMinSize() {
		return screenMinSize;
	}

	uint32_t AcquireNextComputeBuffer() {
		// Save the first index in the compute index array
		uint32_t firstIndex = computeIndices[0];

		// Move every other index back
		for(uint32_t i = 1; i != POINT_BUFFER_COUNT - 1; ++i)
			computeIndices[i - 1] = computeIndices[i];
		
		// Set the first index as the last index
		computeIndices[POINT_BUFFER_COUNT - 2] = firstIndex;

		return computeIndices[0];
	}
	uint32_t AcquireNextGraphicsBuffer() {
		// Save the last index in the compute index array
		uint32_t lastIndex = computeIndices[POINT_BUFFER_COUNT - 2];

		// Move every other index except the first index forward
		for(uint32_t i = POINT_BUFFER_COUNT - 3; i; ++i)
			computeIndices[i + 1] = computeIndices[i];
		
		// Set the new second index
		computeIndices[1] = graphicsIndex;

		// Set the graphics index
		graphicsIndex = lastIndex;

		return graphicsIndex;
	}
	uint32_t GetCurrentComputeBuffer() {
		return computeIndices[0];
	}
	uint32_t GetCurrentGraphicsBuffer() {
		return graphicsIndex;
	}
}