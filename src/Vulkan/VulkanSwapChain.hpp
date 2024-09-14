#pragma once

#include "Debug/Logger.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A wrapper for a window's Vulkan swap chain.
	class VulkanSwapChain {
	public:
		VulkanSwapChain() = delete;
		VulkanSwapChain(const VulkanSwapChain&) = delete;
		VulkanSwapChain(VulkanSwapChain&&) noexcept = delete;

		/// @brief Creates a Vulkan swap chain.
		/// @param device The device to create the swap chain in.
		/// @param surface The surface the swap chain will be attached to.
		VulkanSwapChain(VulkanDevice* device, VulkanSurface* surface);

		VulkanSwapChain operator=(const VulkanSwapChain&) = delete;
		VulkanSwapChain operator=(VulkanSwapChain&&) = delete;

		/// @brief Gets the Vulkan device that owns this swap chain.
		/// @return A pointer to the Vulkan device wrapper object.
		VulkanDevice* GetDevice() {
			return device;
		}
		/// @brief Gets the Vulkan device that owns this swap chain.
		/// @return A const pointer to the Vulkan device wrapper object.
		const VulkanDevice* GetDevice() const {
			return device;
		}
		/// @brief Gets the Vulkan surface that this swap chain is attached to.
		/// @return A pointer to the Vulkan surface wrapper object.
		VulkanSurface* GetSurface() {
			return surface;
		}
		/// @brief Gets the Vulkan surface that this swap chain is attached to.
		/// @return A const pointer to the Vulkan surface wrapper object.
		const VulkanSurface* GetSurface() const {
			return surface;
		}

		/// @brief Gets the implementation's Vulkan swap chain.
		/// @return A handle to the Vulkan swap chain, or VK_NULL_HANDLE if the window is minimized.
		VkSwapchainKHR GetSwapChain() {
			return swapChain;
		}
		/// @brief Returns the swap chain's extent.
		/// @return A struct containing the swap chain's extent.
		VkExtent2D GetSwapChainExtent() const {
			return extent;
		}
		/// @brief Returns the implementation's Vulkan render pass.
		/// @return A handle to the Vulkan render pass.
		VkRenderPass GetRenderPass() {
			return renderPass;
		}
		/// @brief Gets the number of images in the swap chain.
		/// @return The number of images in the swap chain.
		uint32_t GetImageCount() const {
			return imageCount;
		}
		/// @brief Gets the images in the swap chain.
		/// @return A pointer to the array of images in the swap chain.
		VkImage* GetImages() {
			return images;
		}
		/// @brief Gets the image views for the swap chain's images.
		/// @return A pointer to the array of image views for the swap chain's images.
		VkImageView* GetImageViews() {
			return imageViews;
		}
		/// @brief Gets the framebuffers for the swap chain's images.
		/// @return A pointer to the array of framebuffers for the swap chain's images.
		VkFramebuffer* GetFramebuffers() {
			return framebuffers;
		}

		/// @brief Logs relevant info about the swap chain to the given logger.
		/// @param logger The logger to log the swap chain's info to.
		void LogSwapChainInfo(Logger* logger);

		/// @brief Destroys the Vulkan swap chain.
		~VulkanSwapChain();
	private:
		static void WindowResizeCallback(void* userData, void* args);

		void FindSwapChainFormat();
		void CreateRenderPass();
		void CreateSwapChain(VkSwapchainKHR oldSwapChain);
		void RecreateSwapChain();

		VulkanDevice* device;
		VulkanSurface* surface;

		VkFormat format;
		VkColorSpaceKHR colorSpace;
		VkSwapchainKHR swapChain;
		VkExtent2D extent;
		VkRenderPass renderPass;
		uint32_t imageCount;
		VkImage* images;
		VkImageView* imageViews;
		VkFramebuffer* framebuffers;
	};
}