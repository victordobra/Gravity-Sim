#include "Renderer.hpp"
#include "Utils/BuildInfo.hpp"
#include "Core/Allocator.hpp"
#include "Core/CommandPool.hpp"
#include "Core/Device.hpp"
#include "Core/Instance.hpp"
#include "Core/SwapChain.hpp"

namespace gsim {
    // Constants
#ifdef GSIM_BUILD_MODE_DEBUG
    const bool DEBUG_ENABLED = true;
#else
    const bool DEBUG_ENABLED = false;
#endif

    // Public functions
    void CreateVulkanRenderer() {
        // Create the core components
        CreateVulkanInstance(DEBUG_ENABLED);
        CreateVulkanSurface();
        CreateVulkanDevice();
        CreateVulkanCommandPools();
        CreateVulkanSwapChain();
    }
    void DestroyVulkanRenderer() {
        // Destroy the core components
        DestroyVulkanSwapChain();
        DestroyVulkanCommandPools();
        DestroyVulkanDevice();
        DestroyVulkanSurface();
        DestroyVulkanInstance();
    }
}