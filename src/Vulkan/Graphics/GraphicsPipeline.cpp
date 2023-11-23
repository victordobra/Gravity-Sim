#include "GrahpicsPipeline.hpp"
#include "Vulkan/Core/Allocator.hpp"
#include "Vulkan/Core/CommandPool.hpp"
#include "Vulkan/Core/Device.hpp"
#include "Vulkan/Core/RenderPass.hpp"
#include "Vulkan/Core/SwapChain.hpp"
#include "Vulkan/Data/Points.hpp"
#include "Vulkan/Data/SyncObjects.hpp"
#include "Debug/Logger.hpp"
#include "Utils/Point.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Constants
	static const uint32_t VERTEX_SHADER_SOURCE[] {
#include "Shaders/VertexShader.vert.u32"
	};
	static const uint32_t FRAGMENT_SHADER_SOURCE[] {
#include "Shaders/FragmentShader.frag.u32"
	};

	// Structs
	struct PushConstants {
		Vector2 screenPos;
		Vector2 screenSize;
	};

	// Variables
	static VkShaderModule vertexShaderModule;
	static VkShaderModule fragmentShaderModule;
	static VkPipelineLayout pipelineLayout;
	static VkPipeline pipeline;

	static VkSemaphore imageAvailableSemaphore;
	static VkSemaphore renderingFinishedSemaphore;

	static VkCommandBuffer commandBuffer;

	// Internal helper functions
	static void CreateShaderModules() {
		// Set the vertex shader module create info
		VkShaderModuleCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = sizeof(VERTEX_SHADER_SOURCE);
		createInfo.pCode = VERTEX_SHADER_SOURCE;

		// Create the vertex shader module
		VkResult result = vkCreateShaderModule(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &vertexShaderModule);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan vertex shader module! Error code: %s", string_VkResult(result));
		
		// Set the fragment shader module create info
		createInfo.codeSize = sizeof(FRAGMENT_SHADER_SOURCE);
		createInfo.pCode = FRAGMENT_SHADER_SOURCE;

		// Create the vertex shader module
		result = vkCreateShaderModule(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &fragmentShaderModule);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan fragment shader module! Error code: %s", string_VkResult(result));
	}
	static void CreatePipelineLayout() {
		// Set the push constant range info
		VkPushConstantRange pushRange;
		pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushRange.offset = 0;
		pushRange.size = sizeof(PushConstants);

		// Set the pipeline layout create info
		VkPipelineLayoutCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.setLayoutCount = 0;
		createInfo.pSetLayouts = nullptr;
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushRange;

		// Create the pipeline layout
		VkResult result = vkCreatePipelineLayout(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &pipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan graphics pipeline layout! Error code: %s", string_VkResult(result));
	}
	static void CreatePipeline() {
		// Set the shader stage create infos
		VkPipelineShaderStageCreateInfo shaderStageInfos[2];

		shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[0].pNext = nullptr;
		shaderStageInfos[0].flags = 0;
		shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageInfos[0].module = vertexShaderModule;
		shaderStageInfos[0].pName = "main";
		shaderStageInfos[0].pSpecializationInfo = nullptr;

		shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfos[1].pNext = nullptr;
		shaderStageInfos[1].flags = 0;
		shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageInfos[1].module = fragmentShaderModule;
		shaderStageInfos[1].pName = "main";
		shaderStageInfos[1].pSpecializationInfo = nullptr;

		// Set the vertex input binding and attribute descriptions
		VkVertexInputBindingDescription vertexBinding;
		vertexBinding.binding = 0;
		vertexBinding.stride = sizeof(Point);
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertexAttributes[3];

		vertexAttributes[0].location = 0;
		vertexAttributes[0].binding = 0;
		vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[0].offset = offsetof(Point, pos);

		vertexAttributes[1].location = 1;
		vertexAttributes[1].binding = 0;
		vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset = offsetof(Point, vel);

		vertexAttributes[2].location = 2;
		vertexAttributes[2].binding = 0;
		vertexAttributes[2].format = VK_FORMAT_R32_SFLOAT;
		vertexAttributes[2].offset = offsetof(Point, mass);

		// Set the vertex input state create info
		VkPipelineVertexInputStateCreateInfo vertexInputInfo;

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.pNext = nullptr;
		vertexInputInfo.flags = 0;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
		vertexInputInfo.vertexAttributeDescriptionCount = 3;
		vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes;
		
		// Set the input asembly state create info
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;

		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.pNext = nullptr;
		inputAssemblyInfo.flags = 0;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		// Set the viewport state create info
		VkPipelineViewportStateCreateInfo viewportInfo;

		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.pNext = nullptr;
		viewportInfo.flags = 0;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = nullptr;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = nullptr;

		// Set the rasterization state create info
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;

		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.pNext = nullptr;
		rasterizationInfo.flags = 0;
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_POINT;
		rasterizationInfo.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.depthBiasConstantFactor = 0.f;
		rasterizationInfo.depthBiasClamp = 0.f;
		rasterizationInfo.depthBiasSlopeFactor = 0.f;
		rasterizationInfo.lineWidth = 1.f;

		// Set the multisample state create info
		VkPipelineMultisampleStateCreateInfo multisampleInfo;

		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.pNext = nullptr;
		multisampleInfo.flags = 0;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.minSampleShading = 0.f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

		// Set the color blend attachment state
		VkPipelineColorBlendAttachmentState colorBlendAttachment;

		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// Set the color blend state create info
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;

		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.pNext = nullptr;
		colorBlendInfo.flags = 0;
		colorBlendInfo.logicOpEnable = VK_TRUE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachment;
		colorBlendInfo.blendConstants[0] = 0.f;
		colorBlendInfo.blendConstants[1] = 0.f;
		colorBlendInfo.blendConstants[2] = 0.f;
		colorBlendInfo.blendConstants[3] = 0.f;

		// Set the dynamic states
		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		// Set the dynamic state create info
		VkPipelineDynamicStateCreateInfo dynamicInfo;

		dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.pNext = nullptr;
		dynamicInfo.flags = 0;
		dynamicInfo.dynamicStateCount = 2;
		dynamicInfo.pDynamicStates = dynamicStates;

		// Set the pipeline create info
		VkGraphicsPipelineCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.stageCount = 2;
		createInfo.pStages = shaderStageInfos;
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssemblyInfo;
		createInfo.pTessellationState = nullptr;
		createInfo.pViewportState = &viewportInfo;
		createInfo.pRasterizationState = &rasterizationInfo;
		createInfo.pMultisampleState = &multisampleInfo;
		createInfo.pDepthStencilState = nullptr;
		createInfo.pColorBlendState = &colorBlendInfo;
		createInfo.pDynamicState = &dynamicInfo;
		createInfo.layout = pipelineLayout;
		createInfo.renderPass = GetVulkanRenderPass();
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		// Create the pipeline
		VkResult result = vkCreateGraphicsPipelines(GetVulkanDevice(), VK_NULL_HANDLE, 1, &createInfo, GetVulkanAllocCallbacks(), &pipeline);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create Vulkan graphics pipeline! Error code: %s", string_VkResult(result));
	}
	static void CreateSemaphores() {
		// Set the semaphore create info
		VkSemaphoreCreateInfo createInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;

		// Create the image available semaphore
		VkResult result = vkCreateSemaphore(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &imageAvailableSemaphore);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create image available semaphore! Error code: %s", string_VkResult(result));

		// Create the rendering finished semaphore
		result = vkCreateSemaphore(GetVulkanDevice(), &createInfo, GetVulkanAllocCallbacks(), &renderingFinishedSemaphore);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to create rendering finished semaphore! Error code: %s", string_VkResult(result));
	}
	static void AllocCommandBuffer() {
		// Set the command buffer alloc info
		VkCommandBufferAllocateInfo allocInfo;

		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.commandPool = GetVulkanGraphicsCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		// Allocate the command buffer
		VkResult result = vkAllocateCommandBuffers(GetVulkanDevice(), &allocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to allocate Vulkan graphics command buffer! Error code: %s", string_VkResult(result));
	}

	// Public functions
	void CreateGraphicsPipeline() {
		CreateShaderModules();
		CreatePipelineLayout();
		CreatePipeline();
		CreateSemaphores();
		AllocCommandBuffer();

		GSIM_LOG_INFO("Created Vulkan graphics pipeline.");
	}
	void DestroyGraphicsPipeline() {
		// Wait for the device to idle
		VkResult result = vkDeviceWaitIdle(GetVulkanDevice());
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to wait for device idle! Error code: %s", result);
		
		// Free the graphics command buffer
		vkFreeCommandBuffers(GetVulkanDevice(), GetVulkanGraphicsCommandPool(), 1, &commandBuffer);

		// Destroy the semaphores
		vkDestroySemaphore(GetVulkanDevice(), imageAvailableSemaphore, GetVulkanAllocCallbacks());
		vkDestroySemaphore(GetVulkanDevice(), renderingFinishedSemaphore, GetVulkanAllocCallbacks());

		// Destroy the pipeline and its layout
		vkDestroyPipeline(GetVulkanDevice(), pipeline, GetVulkanAllocCallbacks());
		vkDestroyPipelineLayout(GetVulkanDevice(), pipelineLayout, GetVulkanAllocCallbacks());

		// Destroy the shader modules
		vkDestroyShaderModule(GetVulkanDevice(), vertexShaderModule, GetVulkanAllocCallbacks());
		vkDestroyShaderModule(GetVulkanDevice(), fragmentShaderModule, GetVulkanAllocCallbacks());
	}

	VkShaderModule GetVulkanVertexShaderModule() {
		return vertexShaderModule;
	}
	VkShaderModule GetVulkanFragmentShaderModule() {
		return fragmentShaderModule;
	}
	VkPipelineLayout GetVulkanGraphicsPipelineLayout() {
		return pipelineLayout;
	}
	VkPipeline GetVulkanGraphicsPipeline() {
		return pipeline;
	}

	void DrawPoints() {
		// Exit the function if the window is minimized
		if(!GetVulkanSwapChain())
			return;
		
		// Acquire the next graphics buffer index
		uint32_t bufferIndex = AcquireNextGraphicsBuffer();

		VkBuffer pointBuffer = GetPointBuffers()[bufferIndex];
		VkFence fence = GetVulkanGraphicsFence();

		// Reset the graphics fence
		VkResult result = vkResetFences(GetVulkanDevice(), 1, &fence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to reset Vulkan graphics fence! Error code: %s", string_VkResult(result));

		// Acquire the swap chain's next image index
		uint32_t imageIndex;

		result = vkAcquireNextImageKHR(GetVulkanDevice(), GetVulkanSwapChain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to acquire next Vulkan swap chain image! Error code: %s", string_VkResult(result));

		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo;

		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to begin recording Vulkan graphics command buffer! Error code: %s", string_VkResult(result));
		
		// Set the clear value
		VkClearValue clearValue = { 0.f, 0.f, 0.f, 1.f };

		// Set the render pass begin info
		VkRenderPassBeginInfo renderPassInfo;

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.renderPass = GetVulkanRenderPass();
		renderPassInfo.framebuffer = GetVulkanSwapChainImages()[imageIndex].framebuffer;
		renderPassInfo.renderArea = { { 0, 0 }, GetVulkanSwapChainExtent() };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		// Begin the render pass
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set the viewport
		VkViewport viewport;

		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = (float)GetVulkanSwapChainExtent().width;
		viewport.height = (float)GetVulkanSwapChainExtent().height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Set the scissor
		vkCmdSetScissor(commandBuffer, 0, 1, &renderPassInfo.renderArea);

		// Bind the graphics pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind the point buffer
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &pointBuffer, &offset);

		// Calculate the screen's size
		float aspectRatio = (float)GetVulkanSwapChainExtent().width / (float)GetVulkanSwapChainExtent().height;
		float minAspectRatio = (float)GetScreenMinSize().x / (float)GetScreenMinSize().y;

		Vector2 screenSize;

		if(aspectRatio > minAspectRatio) {
			float aspectRatioRatio = aspectRatio / minAspectRatio;
			screenSize = { aspectRatioRatio * GetScreenMinSize().x, GetScreenMinSize().y };
		} else {
			float aspectRatioRatio = minAspectRatio / aspectRatio;
			screenSize = { GetScreenMinSize().x, aspectRatioRatio * GetScreenMinSize().y };
		}

		// Set the push constants
		PushConstants pushConstants = { GetScreenPos(), screenSize };

		// Push the constants to the command buffer
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

		// Draw the points
		vkCmdDraw(commandBuffer, GetPointCount(), 1, 0, 0);

		// End the render pass
		vkCmdEndRenderPass(commandBuffer);

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to end recording Vulkan graphics command buffer! Error code: %s", string_VkResult(result));

		// Set the semaphore wait stages
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Set the submit info
		VkSubmitInfo submitInfo;

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

		// Submit to the graphics queue
		result = vkQueueSubmit(GetVulkanGraphicsQueue(), 1, &submitInfo, fence);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to submit Vulkan graphics command buffer! Error code: %s", string_VkResult(result));

		// Set the present info
		VkSwapchainKHR swapChain = GetVulkanSwapChain();

		VkPresentInfoKHR presentInfo;

		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		// Present the image
		result = vkQueuePresentKHR(GetVulkanPresentQueue(), &presentInfo);
		if(result != VK_SUCCESS)
			GSIM_LOG_FATAL("Failed to present to Vulkan swap chain! Error code: %s", string_VkResult(result));
	}
}