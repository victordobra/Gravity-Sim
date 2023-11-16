#include "SyncObjects.hpp"
#include "Points.hpp"
#include "Debug/Logger.hpp"
#include "Vulkan/Core/Allocator.hpp"
#include "Vulkan/Core/Device.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Internal variables
	VkFence fences[POINT_BUFFER_COUNT];
	VkSemaphore availableSemaphores[POINT_BUFFER_COUNT];
	VkSemaphore finishedSemaphores[POINT_BUFFER_COUNT];

	// Public functions
	void CreateVulkanSyncObjects() {
		// Set the fence create info
		VkFenceCreateInfo fenceCreateInfo;

		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Create the fences
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			VkResult result = vkCreateFence(GetVulkanDevice(), &fenceCreateInfo, GetVulkanAllocCallbacks(), fences + i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan point buffer fence! Error code: %s", string_VkResult(result));
		}

		// Set the semaphore create info
		VkSemaphoreCreateInfo semaphoreCreateInfo;

		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		// Create the semaphores
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			VkResult result = vkCreateSemaphore(GetVulkanDevice(), &semaphoreCreateInfo, GetVulkanAllocCallbacks(), availableSemaphores + i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan point buffer semaphore! Error code: %s", string_VkResult(result));
		}
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i) {
			VkResult result = vkCreateSemaphore(GetVulkanDevice(), &semaphoreCreateInfo, GetVulkanAllocCallbacks(), finishedSemaphores + i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan point buffer semaphore! Error code: %s", string_VkResult(result));
		}

		GSIM_LOG_INFO("Created Vulkan sync objects.");
	}
	void DestroyVulkanSyncObjects() {
		// Destroy the fences
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroyFence(GetVulkanDevice(), fences[i], GetVulkanAllocCallbacks());

		// Destroy the semaphores
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroySemaphore(GetVulkanDevice(), availableSemaphores[i], GetVulkanAllocCallbacks());
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroySemaphore(GetVulkanDevice(), finishedSemaphores[i], GetVulkanAllocCallbacks());
	}

	VkFence* GetVulkanPointBufferFences() {
		return fences;
	}
	VkSemaphore* GetVulkanPointBufferAvailableSemaphores() {
		return availableSemaphores;
	}
	VkSemaphore* GetVulkanPointBufferFinishedSemaphores() {
		return finishedSemaphores;
	}
}