#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
    /// @brief Creates the Vulkan render pass.
    void CreateVulkanRenderPass();
    /// @brief Destroys the Vulkan render pass.
    void DestroyVulkanRenderPass();
    /// @brief Recreates the Vulkan render pass.
    void RecreateVulkanRenderPass();

    /// @brief Gets the Vulkan render pass.
    /// @return A handle to the Vulkan render pass.
    VkRenderPass GetVulkanRenderPass();
}