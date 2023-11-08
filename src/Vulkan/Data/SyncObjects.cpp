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
	VkSemaphore semaphores[POINT_BUFFER_COUNT];

	// Public functions
	void CreateVulkanSyncObjects() {
		// Set the fence create info
		VkFenceCreateInfo fenceCreateInfo;

		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = 0;

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
			VkResult result = vkCreateSemaphore(GetVulkanDevice(), &semaphoreCreateInfo, GetVulkanAllocCallbacks(), semaphores + i);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan point buffer semaphore! Error code: %s", string_VkResult(result));
		}
	}
	void DestroyVulkanSyncObjects() {
		// Destroy the fences
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroyFence(GetVulkanDevice(), fences[i], GetVulkanAllocCallbacks());

		// Destroy the semaphores
		for(uint32_t i = 0; i != POINT_BUFFER_COUNT; ++i)
			vkDestroySemaphore(GetVulkanDevice(), semaphores[i], GetVulkanAllocCallbacks());
	}

	VkFence* GetVulkanPointBufferFences() {
		return fences;
	}
	VkSemaphore* GetVulkanPointBufferSemaphores() {
		return semaphores;
	}
}