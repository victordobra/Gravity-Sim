#include "Renderer.hpp"
#include "Manager/Parser.hpp"
#include "Debug/Logger.hpp"

namespace gsim {
	// Public functions
	void CreateVulkanRenderer() {
		// Create the core components
		CreateVulkanInstance(IsValidationEnabled());
		CreateVulkanSurface();
		CreateVulkanDevice();
		CreateVulkanCommandPools();
		CreateVulkanSwapChain();

		// Create the point buffers
		CreatePointBuffers();
	}
	void DestroyVulkanRenderer() {
		// Destroy the point buffer
		DestroyPointBuffers();

		// Destroy the core components
		DestroyVulkanSwapChain();
		DestroyVulkanCommandPools();
		DestroyVulkanDevice();
		DestroyVulkanSurface();
		DestroyVulkanInstance();

		GSIM_LOG_INFO("Destroyed Vulkan renderer.");
	}
}