#pragma once

#include <stddef.h>
#include <stdint.h>

namespace gsim {
#if defined(__GNUC__)
/// @brief Aligns the current data type with the given number of bytes.
/// @param alignment The alignment of the current structure.
#define GSIM_ALIGNAS(alignment) __attribute__((aligned(alignment)))
#elif defined(_MSC_VER)
/// @brief Aligns the current data type with the given number of bytes.
/// @param alignment The alignment of the current structure.
#define GSIM_ALIGNAS(alignment) __declspec(align(alignment))
#endif

	/// @brief A two-dimensional vector.
	struct Vec2 {
		/// @brief The vector's X component.
		float x;
		/// @brief The vector's Y component.
		float y;
	};
	/// @brief A struct containing the relevand info about a particle.
	struct GSIM_ALIGNAS(sizeof(Vec2)) Particle {
		/// @brief The particle's position.
		Vec2 pos;
		/// @brief The particle's velocity.
		Vec2 vel;
		/// @brief The particle's mass.
		float mass;
		/// @brief The inverse of the particle's mass.
		float invMass;
	};
}