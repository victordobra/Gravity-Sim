#include "GraphicsPipeline.hpp"
#include "Debug/Exception.hpp"
#include <stdint.h>

#include <vulkan/vk_enum_string_helper.h>

namespace gsim {
	// Structs
	struct PushConstants {
		Vec2 cameraPos;
		Vec2 cameraSize;
	};

	// Shader sources
	const uint32_t VERTEX_SHADER_SOURCE[] {
#include "Shaders/VertShader.vert.u32"
	};
	const uint32_t FRAGMENT_SHADER_SOURCE[] {
#include "Shaders/FragShader.frag.u32"
	};

	// Public functions
	GraphicsPipeline::GraphicsPipeline(VulkanDevice* device, VulkanSwapChain* swapChain, ParticleSystem* particleSystem) : device(device), swapChain(swapChain), particleSystem(particleSystem) {
		// Set the vertex shader module create info
		VkShaderModuleCreateInfo vertexShaderModuleInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(VERTEX_SHADER_SOURCE),
			.pCode = VERTEX_SHADER_SOURCE
		};

		// Create the vertex shader module
		VkResult result = vkCreateShaderModule(device->GetDevice(), &vertexShaderModuleInfo, nullptr, &vertexShaderModule);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan vertex shader module! Error code: %s", string_VkResult(result));

		// Set the fragment shader module create info
		VkShaderModuleCreateInfo fragmentShaderModuleInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = sizeof(FRAGMENT_SHADER_SOURCE),
			.pCode = FRAGMENT_SHADER_SOURCE
		};

		// Create the fragment shader module
		result = vkCreateShaderModule(device->GetDevice(), &fragmentShaderModuleInfo, nullptr, &fragmentShaderModule);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan fragment shader module! Error code: %s", string_VkResult(result));

		// Set the push constant range
		VkPushConstantRange pushConstantRange {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(PushConstants)
		};

		// Set the pipeline layout create info
		VkPipelineLayoutCreateInfo layoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};

		// Create the pipeline layout
		result = vkCreatePipelineLayout(device->GetDevice(), &layoutInfo, nullptr, &pipelineLayout);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan graphics pipeline layout! Error code: %s", string_VkResult(result));
		
		// Set the pipeline shader stage infos
		VkPipelineShaderStageCreateInfo shaderStageInfos[] {
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vertexShaderModule,
				.pName = "main",
				.pSpecializationInfo = nullptr
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = fragmentShaderModule,
				.pName = "main",
				.pSpecializationInfo = nullptr
			}
		};

		// Set the vertex input binding and attribure descriptions
		VkVertexInputBindingDescription vertexBinding {
			.binding = 0,
			.stride = sizeof(Vec2),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		VkVertexInputAttributeDescription vertexAttribute {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = 0
		};

		// Set the vertex input state info
		VkPipelineVertexInputStateCreateInfo vertexInputInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertexBinding,
			.vertexAttributeDescriptionCount = 1,
			.pVertexAttributeDescriptions = &vertexAttribute
		};

		// Set the input assembly state info
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		// Set the viewport state info
		VkPipelineViewportStateCreateInfo viewportInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr
		};

		// Set the rasterization state info
		VkPipelineRasterizationStateCreateInfo rasterizationInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_POINT,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};

		// Set the multisample state info
		VkPipelineMultisampleStateCreateInfo multisampleInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		// Set the color blend attachment state
		VkPipelineColorBlendAttachmentState colorBlendAttachment {
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		// Set the color blend state info
		VkPipelineColorBlendStateCreateInfo colorBlendInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_TRUE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		// Set the dynamic states
		VkDynamicState dynamicStates[] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		// Set the dynamic state create info
		VkPipelineDynamicStateCreateInfo dynamicInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates
		};

		// Set the pipeline create info
		VkGraphicsPipelineCreateInfo pipelineInfo {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = 2,
			.pStages = shaderStageInfos,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pTessellationState = nullptr,
			.pViewportState = &viewportInfo,
			.pRasterizationState = &rasterizationInfo,
			.pMultisampleState = &multisampleInfo,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlendInfo,
			.pDynamicState = &dynamicInfo,
			.layout = pipelineLayout,
			.renderPass = swapChain->GetRenderPass(),
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		// Create the pipeline
		result = vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan graphics pipeline! Error code: %s", string_VkResult(result));
		
		// Set the fence create info
		VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		// Create the fence
		result = vkCreateFence(device->GetDevice(), &fenceInfo, nullptr, &renderingFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan rendering synchronization fence! Error code: %s", string_VkResult(result));
		
		// Set the semaphore create info
		VkSemaphoreCreateInfo semaphoreInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// Create the semaphores
		result = vkCreateSemaphore(device->GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan rendering synchronization semaphores! Error code: %s", string_VkResult(result));

		result = vkCreateSemaphore(device->GetDevice(), &semaphoreInfo, nullptr, &renderingFinishedSemaphore);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to create Vulkan rendering synchronization semaphores! Error code: %s", string_VkResult(result));
		
		// Set the command buffer alloc info
		VkCommandBufferAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = device->GetGraphicsCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		// Allocate the command buffer
		result = vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, &commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to allocate Vulkan rendering command buffer! Error code: %s", string_VkResult(result));
	}

	void GraphicsPipeline::RenderParticles(Vec2 cameraPos, Vec2 cameraSize) {
		// Exit the function if the window is minimized
		if(!swapChain->GetSwapChain())
			return;

		// Wait for the previous rendering operation to finish
		VkResult result = vkWaitForFences(device->GetDevice(), 1, &renderingFence, VK_TRUE, UINT64_MAX);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to wait for Vulkan rendering fence! Error code: %s", string_VkResult(result));
		
		// Reset the rendering fence
		result = vkResetFences(device->GetDevice(), 1, &renderingFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to reset Vulkan rendering fence! Error code: %s", string_VkResult(result));
		
		// Acquire the next swap chain image's index
		uint32_t imageIndex;
		result = vkAcquireNextImageKHR(device->GetDevice(), swapChain->GetSwapChain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to acquire next Vulkan swap chain image! Error code: %s", string_VkResult(result));
		
		// Reset the command buffer
		result = vkResetCommandBuffer(commandBuffer, 0);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to reset Vulkan rendering command buffer! Error code: %s", string_VkResult(result));

		// Set the command buffer begin info
		VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		// Begin recording the command buffer
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to begin recording Vulkan rendering command buffer! Error code: %s", string_VkResult(result));
		
		// Set the background clear value
		VkClearValue clearValue { 0.0f, 0.0f, 0.0f, 1.0f };

		// Set the render pass begin info
		VkRenderPassBeginInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = swapChain->GetRenderPass(),
			.framebuffer = swapChain->GetFramebuffers()[imageIndex],
			.renderArea = {
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = swapChain->GetSwapChainExtent()
			},
			.clearValueCount = 1,
			.pClearValues = &clearValue
		};

		// Begin the render pass
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set the viewport
		VkViewport viewport {
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)swapChain->GetSwapChainExtent().width,
			.height = (float)swapChain->GetSwapChainExtent().height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Set the scissor
		VkRect2D scissor {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = swapChain->GetSwapChainExtent()
		};

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Bind the graphics pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind the particle buffer
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(particleSystem->GetBuffers()[particleSystem->GetGrahpicsIndex()].posBuffer), &offset);
		particleSystem->NextGraphicsIndex();

		// Push the constants to the command buffer
		PushConstants pushConstants {
			.cameraPos = cameraPos,
			.cameraSize = cameraSize
		};
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

		// Draw the particles
		vkCmdDraw(commandBuffer, (uint32_t)particleSystem->GetParticleCount(), 1, 0, 0);

		// End the render pass
		vkCmdEndRenderPass(commandBuffer);

		// End recording the command buffer
		result = vkEndCommandBuffer(commandBuffer);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to end recording Vulkan rendering command buffer! Error code: %s", string_VkResult(result));

		// Set the submit info
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &imageAvailableSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderingFinishedSemaphore
		};

		// Submit the command buffer to the graphics queue
		result = vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, renderingFence);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to submit Vulkan rendering command buffer for execution! Error code: %s", string_VkResult(result));
		
		// Set the present info
		VkSwapchainKHR swapChainHandle = swapChain->GetSwapChain();

		VkPresentInfoKHR presentInfo {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderingFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &swapChainHandle,
			.pImageIndices = &imageIndex,
			.pResults = nullptr
		};

		// Present the image
		result = vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);
		if(result != VK_SUCCESS)
			GSIM_THROW_EXCEPTION("Failed to present to Vulkan swap chain! Error code: %s", string_VkResult(result));
	}

	GraphicsPipeline::~GraphicsPipeline() {
		// Wait for the rendering fence
		vkWaitForFences(device->GetDevice(), 1, &renderingFence, VK_TRUE, UINT64_MAX);

		// Destroy the pipeline's objects
		vkFreeCommandBuffers(device->GetDevice(), device->GetGraphicsCommandPool(), 1, &commandBuffer);
		vkDestroyFence(device->GetDevice(), renderingFence, nullptr);
		vkDestroySemaphore(device->GetDevice(), imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device->GetDevice(), renderingFinishedSemaphore, nullptr);
		vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
		vkDestroyShaderModule(device->GetDevice(), vertexShaderModule, nullptr);
		vkDestroyShaderModule(device->GetDevice(), fragmentShaderModule, nullptr);
	}
}