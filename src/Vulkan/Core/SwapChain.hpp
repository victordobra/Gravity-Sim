#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_platform.h>

namespace gsim {
	/// @brief Creates the Vulkan surface.
	void CreateVulkanSurface();
	/// @brief Destroys the Vulkan surface.
	void DestroyVulkanSurface();

	/// @brief Gets the window's Vulkan surface.
	/// @return The handle of the Vulkan surface.
	VkSurfaceKHR GetVulkanSurface();
	/// @brief Gets the Vulkan surface's capabilities.
	/// @return A struct containing the surface's capabilities.
	VkSurfaceCapabilitiesKHR GetVulkanSurfaceCapabilities();
	/// @brief Gets the Vulkan surface's supported formats.
	/// @return A const reference to a vector containing all supported formats.
	const std::vector<VkSurfaceFormatKHR>& GetVulkanSurfaceSupportedFormats();
	/// @brief Gets the Vulkan surface's supported present modes.
	/// @return A const reference to a vector containing all supported present modes.
	const std::vector<VkPresentModeKHR>& GetVulkanSurfaceSupportedPresentModes();

	/// @brief A struct containing the swap chain's default settings.
	struct VulkanSwapChainSettings {
		/// @brief The format of the swap chain's images.
		VkFormat imageFormat;
		/// @brief The color space of the swap chain's images.
		VkColorSpaceKHR imageColorSpace;
		/// @brief The composite alpha flag indicating the alpha composite mode to use.
		VkCompositeAlphaFlagBitsKHR compositeAlpha;
		/// @brief The swap chain's present mode.
		VkPresentModeKHR presentMode;
		/// @brief VK_TRUE if the swap chain can be clipped, otherwise false.
		VkBool32 clipped;
	};
	/// @brief A struct representing a swap chain image.
	struct VulkanSwapChainImage {
		/// @brief The handle of the color image.
		VkImage image;
		/// @brief The handle of the color image's view.
		VkImageView imageView;
		/// @brief The handle of the swap chain image's framebuffer
		VkFramebuffer framebuffer;
	};

	/// @brief Creates the Vulkan swap chain.
	void CreateVulkanSwapChain();
	/// @brief Destroys the Vulkan swap chain.
	void DestroyVulkanSwapChain();
	/// @brief Recreates the Vulkan swap chain.
	void RecreateVulkanSwapChain();

	/// @brief Gets the Vulkan swap chain's settings.
	/// @return A struct containing the swap chain's settings.
	const VulkanSwapChainSettings& GetVulkanSwapChainSettings();
	/// @brief Sets the Vulkan swap chain's new settings.
	/// @param newSettings The swap chain's new settings.
	/// @return True if the given settings are supported by the swap chain, otherwise false.
	bool SetVulkanSwapChainSettings(const VulkanSwapChainSettings& newSettings);
	/// @brief Gets the Vulkan swap chain's extent.
	/// @return The Vulkan swap chain's extent.
	VkExtent2D GetVulkanSwapChainExtent();

	/// @brief Gets the Vulkan swap chain.
	/// @return The handle of the Vulkan swap chain, or VK_NULL_HANDLE if the window is minimized and the swap chain doesn't exist.
	VkSwapchainKHR GetVulkanSwapChain();
	/// @brief Gets the Vulkan swap chain's images.
	/// @return A const reference to a vector containing all swap chain images.
	const std::vector<VulkanSwapChainImage>& GetVulkanSwapChainImages();
}