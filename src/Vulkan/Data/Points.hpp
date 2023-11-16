#pragma once

#include "Utils/Point.hpp"

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

	/// @brief Gets the number of points.
	/// @return The number of points.
	size_t GetPointCount();
	/// @brief Gets the Vulkan point buffers.
	/// @return A pointer to an array containing POINT_BUFFER_COUNT buffers.
	VkBuffer* GetPointBuffers();

	/// @brief Gets the screen's position.
	/// @return The screen's position.
	Vector2 GetScreenPos();
	/// @brief Gets the screen's minimum screen size based on the point's starting positions.
	/// @return The minimum screen size.
	Vector2 GetScreenMinSize();

	/// @brief Acquires the next Vulkan compute point buffer.
	/// @return The index of the next Vulkan compute point buffer.
	uint32_t AcquireNextComputeBuffer();
	/// @brief Acquires the next Vulkan graphics point buffer.
	/// @return The index of the next Vulkan graphics point buffer.
	uint32_t AcquireNextGraphicsBuffer();
	/// @brief Gets the current Vulkan compute point buffer.
	/// @return The index of the current Vulkan compute point buffer.
	uint32_t GetCurrentComputeBuffer();
	/// @brief Gets the current Vulkan graphics point buffer.
	/// @return The index of the current Vulkan graphics point buffer.
	uint32_t GetCurrentGraphicsBuffer();
}