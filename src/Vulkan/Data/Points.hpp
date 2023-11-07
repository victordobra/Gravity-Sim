#pragma once

#include <stdint.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace gsim {
	/// @brief The number of point buffers
	const uint32_t POINT_BUFFER_COUNT = 3;

	/// @brief Creates the Vulkan buffers used to store point coordinates.
	void CreatePointBuffers();
	/// @brief Destroy the Vulkan buffers used to store point coordinates.
	void DestroyPointBuffers();

	/// @brief Gets the Vulkan point buffers.
	/// @return A pointer to an array containing POINT_BUFFER_COUNT buffers.
	VkBuffer* GetPointBuffers();
	/// @brief Acquires the next Vulkan compute point buffer.
	/// @return The index of the next Vulkan compute point buffer.
	uint32_t AcquireNextComputeBuffer();
	/// @brief Acquires the next Vulkan graphics point buffer.
	/// @return The index of the next Vulkan graphics point buffer.
	uint32_t AcquireNextGraphicsBuffer();
}