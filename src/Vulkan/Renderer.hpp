#pragma once

#include "Core/Allocator.hpp"
#include "Core/CommandPool.hpp"
#include "Core/Device.hpp"
#include "Core/Instance.hpp"
#include "Core/RenderPass.hpp"
#include "Core/SwapChain.hpp"
#include "Data/Points.hpp"
#include "Data/SyncObjects.hpp"
#include "Graphics/GrahpicsPipeline.hpp"

namespace gsim {
	/// @brief Creates the Vulkan renderer.
	void CreateVulkanRenderer();
	/// @brief Destroys the Vulkan renderer
	void DestroyVulkanRenderer();
}