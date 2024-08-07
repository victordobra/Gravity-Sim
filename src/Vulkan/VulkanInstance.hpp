#pragma once

#include "Debug/Logger.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
    /// @brief A wrapper for the Vulkan instance and a debug messenger, if requested.
    class VulkanInstance {
    public:
        VulkanInstance() = delete;
        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance(VulkanInstance&&) noexcept = delete;

        /// @brief Creates a Vulkan instance.
        /// @param validationEnabled True if validation can be enabled, otherwise false.
        /// @param logger The logger to log validation messages to.
        VulkanInstance(bool validationEnabled, Logger* logger);

        VulkanInstance& operator=(const VulkanInstance&) = delete;
        VulkanInstance& operator=(VulkanInstance&&) = delete;

        /// @brief Gets the Vulkan instance of the implementation.
        /// @return A handle to the Vulkan instance.
        VkInstance GetInstance() {
            return instance;
        }
        /// @brief Gets the Vulkan debug messenger for the instance.
        /// @return A handle to the Vulkan debug messenger, or VK_NULL_HANDLE if debugging is disabled.
        VkDebugUtilsMessengerEXT GetDebugMessenger() {
            return debugMessenger;
        }

        /// @brief Destroys the Vulkan instance.
        ~VulkanInstance();
    private:
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
    };
}