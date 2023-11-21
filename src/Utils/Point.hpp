#pragma once

#include "Alignment.hpp"

namespace gsim {
	/// @brief A struct representing a 2D vector.
	struct Vector2 {
		/// @brief The X dimension of the vector.
		double x;
		/// @brief The Y dimension of the vector.
		double y;
	};

	/// @brief A struct representing a simulated point.
	struct GSIM_ALIGNAS(sizeof(Vector2)) Point {
		/// @brief The position of the point.
		Vector2 pos;
		/// @brief The velocity of the point.
		Vector2 vel;
		/// @brief The mass of the point.
		double mass;
	};
}