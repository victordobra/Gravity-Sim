#include "VulkanSurface.hpp"
#include "Debug/Exception.hpp"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Public functions
	VulkanSurface::VulkanSurface(VulkanInstance* instance, Window* window) : instance(instance), window(window) {
		// Load the window's platform info
		Window::PlatformInfo platformInfo = window->GetPlatformInfo();

#if defined(WIN32)
		// Set the Win32 surface create info
		VkWin32SurfaceCreateInfoKHR createInfo {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.hinstance = platformInfo.hInstance,
			.hwnd = platformInfo.hWnd
		};

		// Create the Win32 surface
		VkResult result = vkCreateWin32SurfaceKHR(instance->GetInstance(), &createInfo, nullptr, &surface);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan surface! Error code: %s", string_VkResult(result));
#endif
	}

	VulkanSurface::~VulkanSurface() {
		// Destroy the surface
		vkDestroySurfaceKHR(instance->GetInstance(), surface, nullptr);
	}
}