#pragma once

#include <stdint.h>
#include "Utils/Point.hpp"

namespace gsim {
	/// @brief Loads all points from the specified file.
	/// @param fileName The name of the file to read from.
	/// @param points A pointer to the array of points to write to, or a nullptr.
	/// @return The number of points.
	size_t LoadPoints(const char* fileName, Point* points);
}