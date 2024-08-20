#include "VulkanSwapChain.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>
#include <stdlib.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Public functions
	VulkanSwapChain::VulkanSwapChain(VulkanDevice* device, VulkanSurface* surface) : device(device), surface(surface) {
		// Get the number of supported formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface->GetSurface(), &formatCount, nullptr);

		// Allocate the format array
		VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
		if(!formats)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan surface format array!");
		
		VkPresentModeKHR* presentModes = (VkPresentModeKHR*)(formats + formatCount);
		
		// Gets all supported formats
		vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface->GetSurface(), &formatCount, formats);

		// Find the best possible Vulkan surface format
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR colorSpace;
		for(uint32_t i = 0; i != formatCount; ++i) {
			// Skip the current format if it is not viable
			if(formats[i].format != VK_FORMAT_R8G8B8A8_SRGB && formats[i].format != VK_FORMAT_B8G8R8A8_SRGB)
				continue;
			
			// Set the new format and color space
			format = formats[i].format;
			colorSpace = formats[i].colorSpace;

			// Exit the loop if the color space is also preferred
			if(colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				break;
		}

		// Throw an exception if no supported formats were found
		if(format == VK_FORMAT_UNDEFINED)
			GSIM_THROW_EXCEPTION("Fained to find supported Vulkan swap chain format!");
		
		// Free the format array
		free(formats);

		// Get the surface's capabilities
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface->GetSurface(), &capabilities);
		
		// Set the swap chain's extent
		if(capabilities.currentExtent.width == UINT32_MAX || capabilities.currentExtent.height == UINT32_MAX) {
			// Set the swap chain's extent to the window size
			extent.width = surface->GetWindow()->GetWindowInfo().width;
			extent.height = surface->GetWindow()->GetWindowInfo().height;

			// Clamp the extents within the surface's limits
			if(extent.width < capabilities.minImageExtent.width) {
				extent.width = capabilities.minImageExtent.width;
			} else if(extent.width > capabilities.maxImageExtent.width) {
				extent.width = capabilities.maxImageExtent.width;
			}

			if(extent.height < capabilities.minImageExtent.height) {
				extent.height = capabilities.minImageExtent.height;
			} else if(extent.height > capabilities.maxImageExtent.height) {
				extent.height = capabilities.maxImageExtent.height;
			}
		} else {
			// Set the swap chain's extent to the surface's extent
			extent = capabilities.currentExtent;
		}

		// Set the swap chain's minimum image count
		uint32_t minImageCount = capabilities.minImageCount + 1;
		if(minImageCount > capabilities.maxImageCount)
			minImageCount = capabilities.maxImageCount;
		
		// Set the swap chain's queue family info
		VulkanDevice::QueueFamilyIndices indices = device->GetQueueFamilyIndices();

		uint32_t indicesArr[] { indices.graphicsIndex, indices.presentIndex };
		uint32_t indexCount;
		VkSharingMode sharingMode;

		if(indices.graphicsIndex == indices.presentIndex) {
			indexCount = 1;
			sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		} else {
			indexCount = 2;
			sharingMode = VK_SHARING_MODE_CONCURRENT;
		}

		// Set the swap chain's create info
		VkSwapchainCreateInfoKHR swapChainInfo {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = surface->GetSurface(),
			.minImageCount = minImageCount,
			.imageFormat = format,
			.imageColorSpace = colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = sharingMode,
			.queueFamilyIndexCount = indexCount,
			.pQueueFamilyIndices = indicesArr,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};

		// Create the swap chain
		VkResult result = vkCreateSwapchainKHR(device->GetDevice(), &swapChainInfo, nullptr, &swapChain);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan swap chain! Error code: %s", string_VkResult(result));
		
		// Get the swap chain's image count
		vkGetSwapchainImagesKHR(device->GetDevice(), swapChain, &imageCount, nullptr);

		// Allocate all required arrays
		images = (VkImage*)malloc(imageCount * (sizeof(VkImage) + sizeof(VkImageView) + sizeof(VkFramebuffer)));
		if(!images)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan swap chain image array!");
		
		imageViews = (VkImageView*)(images + imageCount);
		framebuffers = (VkFramebuffer*)(imageViews + imageCount);

		// Get the swap chain's images
		vkGetSwapchainImagesKHR(device->GetDevice(), swapChain, &imageCount, images);

		// Set the image view create info, except for the image
		VkImageViewCreateInfo imageViewInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		// Create every swap chain image's view
		for(uint32_t i = 0; i != imageCount; ++i) {
			// Set the image in the view's create info
			imageViewInfo.image = images[i];

			// Create the image view
			result = vkCreateImageView(device->GetDevice(), &imageViewInfo, nullptr, imageViews + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan swap chain image view! Error code: %s", string_VkResult(result));
		}

		// Set the attachment description
		VkAttachmentDescription attachment {
			.flags = 0,
			.format = format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		
		// Set the attachment reference
		VkAttachmentReference attachmentRef {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		// Set the subpass description
		VkSubpassDescription subpass {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentRef,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr
		};

		// Set the render pass create info
		VkRenderPassCreateInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = 1,
			.pAttachments = &attachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 0,
			.pDependencies = nullptr
		};

		// Create the render pass
		result = vkCreateRenderPass(device->GetDevice(), &renderPassInfo, nullptr, &renderPass);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan render pass! Error code: %s", string_VkResult(result));
		
		// Set the framebuffer create info except for its attachment
		VkFramebufferCreateInfo framebufferInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = 1,
			.width = extent.width,
			.height = extent.height,
			.layers = 1
		};

		// Create the framebuffers
		for(uint32_t i = 0; i != imageCount; ++i) {
			// Set the attachment in the framebuffer's create info
			framebufferInfo.pAttachments = imageViews + i;

			// Create the framebuffer
			result = vkCreateFramebuffer(device->GetDevice(), &framebufferInfo, nullptr, framebuffers + i);
			if(result != VK_SUCCESS)
				GSIM_THROW_EXCEPTION("Failed to create Vulkan framebuffer! Error code: %s", string_VkResult(result));
		}
	}

	VulkanSwapChain::~VulkanSwapChain() {
		// Destroy the framebuffers
		for(uint32_t i = 0; i != imageCount; ++i)
			vkDestroyFramebuffer(device->GetDevice(), framebuffers[i], nullptr);
		
		// Destroy the render pass
		vkDestroyRenderPass(device->GetDevice(), renderPass, nullptr);

		// Destroy the image views
		for(uint32_t i = 0; i != imageCount; ++i)
			vkDestroyImageView(device->GetDevice(), imageViews[i], nullptr);
		
		// Destroy the swap chain
		vkDestroySwapchainKHR(device->GetDevice(), swapChain, nullptr);

		// Free the image arrays
		free(images);
	}
}