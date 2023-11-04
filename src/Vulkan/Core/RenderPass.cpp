#include "RenderPass.hpp"
#include "Allocator.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include "Debug/Logger.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Variables
	VkRenderPass renderPass;

	// Public functions
	void CreateVulkanRenderPass() {
		// Set the color attachemnt description
		VkAttachmentDescription attachment;
		
		attachment.flags = 0;
		attachment.format = GetVulkanSwapChainSettings().imageFormat;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Set the color attachment reference
		VkAttachmentReference attachmentRef;

		attachmentRef.attachment = 0;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Set the subpass description
		VkSubpassDescription subpass;

		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentRef;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		// Set the render pass create info
		VkRenderPassCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &attachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 0;
		createInfo.pDependencies = nullptr;

		// Create the render pass
		VkResult result = vkCreateRenderPass(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &renderPass);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan render pass! Error code: %s", string_VkResult(result));
	}
	void DestroyVulkanRenderPass() {
		// Destroy the render pass
		vkDestroyRenderPass(GetVulkanDevice(), renderPass, GetVulkanAllocCallbacks());
	}
	void RecreateVulkanRenderPass() {
		// Destroy and recreate the render pass
		DestroyVulkanRenderPass();
		CreateVulkanRenderPass();
	}

	VkRenderPass GetVulkanRenderPass() {
		return renderPass;
	}
}