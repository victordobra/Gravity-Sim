#include "VulkanCommandPool.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
    // Public functions
	VulkanCommandPool::VulkanCommandPool(VulkanDevice* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) : device(device) {
        // Set the command pool create info
        VkCommandPoolCreateInfo createInfo;

        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = createFlags;
        createInfo.queueFamilyIndex = queueFamilyIndex;

        // Create the command pool
        VkResult result = vkCreateCommandPool(device->GetDevice(), &createInfo, nullptr, &commandPool);
        if(result != VK_SUCCESS)
            GSIM_THROW_EXCEPTION("Failed to create Vulkan command pool! Error code: %s", string_VkResult(result));
    }

    VulkanCommandPool::~VulkanCommandPool() {
        // Destroy the command pool
        vkDestroyCommandPool(device->GetDevice(), commandPool, nullptr);
    }
}