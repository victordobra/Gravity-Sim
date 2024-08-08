#pragma once

#include "Platform/Window.hpp"
#include "VulkanInstance.hpp"
#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief A wrapper for a window's Vulkan surface.
	class VulkanSurface {
	public:
		VulkanSurface() = delete;
		VulkanSurface(const VulkanSurface&) = delete;
		VulkanSurface(VulkanSurface&&) noexcept = delete;

		/// @brief Creates a Vulkan surface.
		/// @param instance The Vulkan instance to create the surface in.
		/// @param window The window to create the surface for.
		VulkanSurface(VulkanInstance* instance, Window* window);

		VulkanSurface& operator=(const VulkanSurface&) = delete;
		VulkanSurface& operator=(VulkanSurface&&) = delete;

		/// @brief Gets the Vulkan instance that owns the surface.
		/// @return A pointer to the Vulkan instance wrapper object.
		VulkanInstance* GetInstance() {
			return instance;
		}
		/// @brief Gets the Vulkan instance that owns the surface.
		/// @return A const pointer to the Vulkan instance wrapper object.
		const VulkanInstance* GetInstance() const {
			return instance;
		}
		/// @brief Gets the window that owns the surface.
		/// @return A pointer to the window object.
		Window* GetWindow() {
			return window;
		}
		/// @brief Gets the window that owns the surface.
		/// @return A const pointer to the window object.
		const Window* GetWindow() const {
			return window;
		}
		/// @brief Gets the Vulkan surface of the implementation.
		/// @return A handle to the Vulkan surface.
		VkSurfaceKHR GetSurface() {
			return surface;
		}

		/// @brief Destroys the Vulkan surface.
		~VulkanSurface();
	private:
		VulkanInstance* instance;
		Window* window;
		VkSurfaceKHR surface;
	};
}