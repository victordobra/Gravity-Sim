#include "SyncObjects.hpp"
#include "Points.hpp"
#include "Debug/Logger.hpp"
#include "Vulkan/Core/Allocator.hpp"
#include "Vulkan/Core/Device.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Internal variables
	VkFence graphicsFence;
	VkFence computeFence;

	// Public functions
	void CreateVulkanSyncObjects() {
		// Set the fence create info
		VkFenceCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Create the graphics fence
		VkResult result = vkCreateFence(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &graphicsFence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan graphics fence! Error code: %s", string_VkResult(result));

		// Create the compute fence
		result = vkCreateFence(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &computeFence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan compute fence! Error code: %s", string_VkResult(result));
		
		GSIM_LOG_INFO("Created Vulkan sync objects.");
	}
	void DestroyVulkanSyncObjects() {
		// Destroy the fences
		vkDestroyFence(GetVulkanDevice(), graphicsFence, GetVulkanAllocCallbacks());
		vkDestroyFence(GetVulkanDevice(), computeFence, GetVulkanAllocCallbacks());
	}

	VkFence GetVulkanGraphicsFence() {
		return graphicsFence;
	}
	VkFence GetVulkanComputeFence() {
		return computeFence;
	}

	void WaitForVulkanFences() {
		// Set the array of fence handles
		VkFence fences[] { graphicsFence, computeFence };

		// Wait for the fences
		VkResult result = vkWaitForFences(GetVulkanDevice(), 2, fences, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for Vulkan fences! Error code: %s", string_VkResult(result));
	}
}