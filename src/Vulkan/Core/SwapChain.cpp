#include "SwapChain.hpp"
#include "Allocator.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "RenderPass.hpp"
#include "Vulkan/Data/Points.hpp"
#include "Vulkan/Data/SyncObjects.hpp"
#include "Platform/Window.hpp"
#include "Debug/Logger.hpp"
#include "Manager/Parser.hpp"
#include <stdint.h>
#include <vector>

// Include Vulkan with the current platform define
#if defined(GSIM_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(GSIM_PLATFORM_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Internal variables
	static VkSurfaceKHR surface;

	static VkSurfaceCapabilitiesKHR surfaceCapabilities;
	static std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats;
	static std::vector<VkPresentModeKHR> supportedSurfacePresentModes;

	static VulkanSwapChainSettings swapChainSettings;
	static VkExtent2D swapChainExtent;
	static VkSwapchainKHR swapChain;
	static std::vector<VulkanSwapChainImage> swapChainImages;

	// Internal functions
	static void GetSwapChainSupportDetails() {
		// Get the surface's supported formats
		uint32_t supportedSurfaceFormatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanPhysicalDevice(), surface, &supportedSurfaceFormatCount, nullptr);
		supportedSurfaceFormats.resize(supportedSurfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanPhysicalDevice(), surface, &supportedSurfaceFormatCount, supportedSurfaceFormats.data());

		// Get the surface's supported present modes
		uint32_t supportedSurfacePresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(GetVulkanPhysicalDevice(), surface, &supportedSurfacePresentModeCount, nullptr);
		supportedSurfacePresentModes.resize(supportedSurfacePresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(GetVulkanPhysicalDevice(), surface, &supportedSurfacePresentModeCount, supportedSurfacePresentModes.data());

		// Shrink the supported surface formats and present modes vectors to fit
		supportedSurfaceFormats.shrink_to_fit();
		supportedSurfacePresentModes.shrink_to_fit();
	}
	static void SetSwapChainDefaultSettings() {
		// Set the swap chain's surface format based on their scores
		swapChainSettings.imageFormat = supportedSurfaceFormats[0].format;
		swapChainSettings.imageColorSpace = supportedSurfaceFormats[0].colorSpace;

		uint32_t maxScore = 0;

		for(const auto& surfaceFormat : supportedSurfaceFormats) {
			// Set the current surface format's score
			uint32_t score = 0;

			// Increase the surface format's score if the format is optimal
			score += surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB || surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB;

			// Increase the surface format's score if the color space is optimal
			score += surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

			// Check if the current score is the maximum score
			if(score > maxScore) {
				// Set the new max score and surface format
				maxScore = score;
				swapChainSettings.imageFormat = surfaceFormat.format;
				swapChainSettings.imageColorSpace = surfaceFormat.colorSpace;
			}
		}

		// Set the swap chain's present mode to FIFO, as it's guaranteed to be supported
		swapChainSettings.presentMode = VK_PRESENT_MODE_FIFO_KHR;

		// Set the swap chain's other settings
		swapChainSettings.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainSettings.clipped = VK_TRUE;
	}
	static void CreateSwapChain(VkSwapchainKHR oldSwapChain) {
		// Get the surface's capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetVulkanPhysicalDevice(), surface, &surfaceCapabilities);

		// Set the swap chain's extent
		if(surfaceCapabilities.currentExtent.width == 0xffffffff || surfaceCapabilities.currentExtent.height == 0xffffffff) {
			// Set the swap chain extent to the window's size
			WindowInfo windowInfo = GetWindowInfo();

			swapChainExtent.width = windowInfo.width;
			swapChainExtent.height = windowInfo.height;

			// Clamp the extent's width and height between the extent limits
			if(swapChainExtent.width < surfaceCapabilities.minImageExtent.width)
				swapChainExtent.width = surfaceCapabilities.minImageExtent.width;
			else if(swapChainExtent.width > surfaceCapabilities.maxImageExtent.width)
				swapChainExtent.width = surfaceCapabilities.maxImageExtent.width;

			if(swapChainExtent.height < surfaceCapabilities.minImageExtent.height)
				swapChainExtent.height = surfaceCapabilities.minImageExtent.height;
			else if(swapChainExtent.height > surfaceCapabilities.maxImageExtent.height)
				swapChainExtent.height = surfaceCapabilities.maxImageExtent.height;
		} else {
			// Use the surface's extent
			swapChainExtent = surfaceCapabilities.currentExtent;
		}

		// Set the swapchain's handle to VK_NULL_HANDLE and exit the function if the window is minimized
		if(!swapChainExtent.width || !swapChainExtent.height) {
			swapChain = VK_NULL_HANDLE;
			return;
		}

		// Set the swap chain's min image count
		uint32_t swapChainMinImageCount = surfaceCapabilities.minImageCount + 1;
		if(swapChainMinImageCount > surfaceCapabilities.maxImageCount)
			swapChainMinImageCount = surfaceCapabilities.maxImageCount;
		
		// Get the physical device's queue family indices
		VulkanQueueFamilyIndices queueFamilyIndices = GetVulkanDeviceQueueFamilyIndices();

		// Set the swap chain's create info
		VkSwapchainCreateInfoKHR createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.surface = surface;
		createInfo.minImageCount = swapChainMinImageCount;
		createInfo.imageFormat = swapChainSettings.imageFormat;
		createInfo.imageColorSpace = swapChainSettings.imageColorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		// Set the swap chain's queue family-related info
		uint32_t swapChainQueueFamilyIndices[]{ queueFamilyIndices.graphicsQueueIndex, queueFamilyIndices.presentQueueIndex };

		if(queueFamilyIndices.graphicsQueueIndex != queueFamilyIndices.presentQueueIndex) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = swapChainQueueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 1;
			createInfo.pQueueFamilyIndices = swapChainQueueFamilyIndices;
		}

		createInfo.preTransform = surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = swapChainSettings.compositeAlpha;
		createInfo.presentMode = swapChainSettings.presentMode;
		createInfo.clipped = swapChainSettings.clipped;
		createInfo.oldSwapchain = oldSwapChain;

		// Create the swap chain
		VkResult result = vkCreateSwapchainKHR(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &swapChain);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan swap chain! Error code: %s", string_VkResult(result));
	}
	static void GetSwapChainImages() {
		// Exit the function if the swap chain wasn't created due to the window being minimized
		if(!swapChain)
			return;

		// Get the swap chain's image count
		uint32_t swapChainImageCount;
		vkGetSwapchainImagesKHR(GetVulkanDevice(), swapChain, &swapChainImageCount, nullptr);

		// Allocate the swap chain image array and fill it
		VkImage* swapChainImagesArr = (VkImage*)malloc(sizeof(VkImage) * swapChainImageCount);
		if(!swapChainImagesArr)
			GSIM_LOG_FATAL("Failed to allocate swap chain image array!");
		
		vkGetSwapchainImagesKHR(GetVulkanDevice(), swapChain, &swapChainImageCount, swapChainImagesArr);

		// Copy the images to the swap chain image vector
		swapChainImages.resize(swapChainImageCount);
		swapChainImages.shrink_to_fit();
		for(uint32_t i = 0; i != swapChainImageCount; ++i)
			swapChainImages[i].image = swapChainImagesArr[i];
		
		// Free the swap chain image array
		free(swapChainImagesArr);

		// Set the swap image views's create infos
		VkImageViewCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainSettings.imageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// Create the image views
		for(auto& swapChainImage : swapChainImages) {
			// Set the target image in the create info
			createInfo.image = swapChainImage.image;
			
			// Create the image view
			VkResult result = vkCreateImageView(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &swapChainImage.imageView);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan swap chain image view! Error code: %s", string_VkResult(result));
		} 
	}
	static void CreateFramebuffers() {
		// Exit the function if the swap chain wasn't created due to the window being minimized
		if(!swapChain)
			return;

		// Set the framebuffer create info
		VkFramebufferCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = GetVulkanRenderPass();
		createInfo.attachmentCount = 1;
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		// Create the framebuffers
		for(auto& swapChainImage : swapChainImages) {
			// Set the framebuffer attachment
			createInfo.pAttachments = &swapChainImage.imageView;

			// Create the framebuffer
			VkResult result = vkCreateFramebuffer(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &swapChainImage.framebuffer);
			if(result != VK_SUCCESS)
				GSIM_LOG_FATAL("Failed to create Vulkan swap chain framebuffer! Error code: %s", string_VkResult(result));
		}
	}

	static void DestroySwapChainImages() {
		// Exit the function if the swap chain wasn't created due to the window being minimized
		if(!swapChain)
			return;

		// Destroy the swap chain's images
		for(auto& swapChainImage : swapChainImages) {
			// Destroy the framebuffer
			vkDestroyFramebuffer(GetVulkanDevice(), swapChainImage.framebuffer, GetVulkanAllocCallbacks());

			// Destroy the image view
			vkDestroyImageView(GetVulkanDevice(), swapChainImage.imageView, GetVulkanAllocCallbacks());
		}

		// Resize the swap chain image vector to 0
		swapChainImages.resize(0);
	}

	// Resize event callback
	static void* ResizeEventCallback(void* params) {
		// Recreate the swap chain
		RecreateVulkanSwapChain();

		return nullptr;
	}

	// Public functions
	void CreateVulkanSurface() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Get the window platform info
		WindowPlatformInfo platformInfo = GetWindowPlatformInfo();

#if defined(GSIM_PLATFORM_WINDOWS)
		// Set the Win32 surface create info
		VkWin32SurfaceCreateInfoKHR createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.hinstance = platformInfo.hInstance;
		createInfo.hwnd = platformInfo.hWindow;

		// Create the Win32 surface
		VkResult result = vkCreateWin32SurfaceKHR(GetVulkanInstance(), &createInfo, GetVulkanAllocCallbacks(), &surface);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Win32 Vulkan window surface! Error code: %s", string_VkResult(result));
#elif defined(GSIM_PLATFORM_LINUX)
		// Set the XCB surface create info
		VkXcbSurfaceCreateInfoKHR createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.connection = platformInfo.connection;
		createInfo.window = platformInfo.window;

		// Create the XCB surface
		VkResult result = vkCreateXcbSurfaceKHR(GetVulkanInstance(), &createInfo, GetVulkanAllocCallbacks(), &surface);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create XCB Vulkan window surface! Error code: %s", string_VkResult(result));
#endif
	}
	void DestroyVulkanSurface() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Destroy the surface
		vkDestroySurfaceKHR(GetVulkanInstance(), surface, GetVulkanAllocCallbacks());
	}

	VkSurfaceKHR GetVulkanSurface() {
		return surface;
	}
	VkSurfaceCapabilitiesKHR GetVulkanSurfaceCapabilities() {
		return surfaceCapabilities;
	}
	const std::vector<VkSurfaceFormatKHR>& GetVulkanSurfaceSupportedFormats() {
		return supportedSurfaceFormats;
	}
	const std::vector<VkPresentModeKHR>& GetVulkanSurfaceSupportedPresentModes() {
		return supportedSurfacePresentModes;
	}

	/// @brief Creates the Vulkan swap chain.
	void CreateVulkanSwapChain() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Set the swap chain's default settings
		GetSwapChainSupportDetails();
		SetSwapChainDefaultSettings();

		// Create the render pass
		CreateVulkanRenderPass();

		// Create the swap chain and its components
		CreateSwapChain(VK_NULL_HANDLE);
		GetSwapChainImages();
		CreateFramebuffers();

		// Add the resize event callback
		GetWindowResizeEvent().AddListener(ResizeEventCallback);
	}
	/// @brief Destroys the Vulkan swap chain.
	void DestroyVulkanSwapChain() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Destroy the swap chain's images
		DestroySwapChainImages();

		// Destroy the swap chain, if it exists
		if(swapChain)
			vkDestroySwapchainKHR(GetVulkanDevice(), swapChain, GetVulkanAllocCallbacks());
		
		// Destroy the render pass
		DestroyVulkanRenderPass();
	}
	void RecreateVulkanSwapChain() {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return;

		// Wait for the graphics and compute fences
		WaitForVulkanFences();
		
		// Wait for the present queue to idle
		VkResult result = vkQueueWaitIdle(GetVulkanPresentQueue());
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for Vulkan present queue! Error code: %s", string_VkResult(result));

		// Destroy the swap chain's previous images
		DestroySwapChainImages();

		// Save the old swap chain's handle
		VkSwapchainKHR oldSwapChain = swapChain;

		// Recreate the swap chain
		CreateSwapChain(oldSwapChain);

		// Destroy the old swap chain, if it exists
		if(oldSwapChain)
			vkDestroySwapchainKHR(GetVulkanDevice(), oldSwapChain, GetVulkanAllocCallbacks());

		// Create the swap chain's remaining components
		GetSwapChainImages();
		CreateFramebuffers();
	}

	const VulkanSwapChainSettings& GetVulkanSwapChainSettings() {
		return swapChainSettings;
	}
	bool SetVulkanSwapChainSettings(const VulkanSwapChainSettings& newSettings) {
		// Exit the function if rendering is not enabled
		if(!IsRenderingEnabled())
			return false;

		// Check if the settings' surface format is supported
		bool surfaceFormatFound = false;
		for(const auto& surfaceFormat : supportedSurfaceFormats) {
			// Check if the settings' surface format is equal to the current surface format
			if(surfaceFormat.format == newSettings.imageFormat && surfaceFormat.colorSpace == newSettings.imageColorSpace) {
				// Save that a surface format was found and exit the loop
				surfaceFormatFound = true;
				break;
			}
		}

		// Exit the function if the given surface format is not supported
		if(!surfaceFormatFound)
			return false;

		// Check if the settings' present mode is supported
		bool presentModeFound = false;
		for(auto presentMode : supportedSurfacePresentModes) {
			// Check if the settings' present mode is equal to the current present mode 
			if(presentMode == newSettings.presentMode) {
				// Save that a present mode was found and exit the loop
				presentModeFound = true;
				break;
			}
		}

		// Exit the function if the given present mode is not supported
		if(!presentModeFound)
			return false;
		
		// The settings are supported; recreate the swap chain and render pass and exit the function
		swapChainSettings = newSettings;
		RecreateVulkanRenderPass();
		RecreateVulkanSwapChain();

		return true;
	}
	VkExtent2D GetVulkanSwapChainExtent() {
		return swapChainExtent;
	}

	VkSwapchainKHR GetVulkanSwapChain() {
		return swapChain;
	}
	const std::vector<VulkanSwapChainImage>& GetVulkanSwapChainImages() {
		return swapChainImages;
	}
}