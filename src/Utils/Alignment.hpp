#pragma once

#include <stddef.h>

#if defined(__GNUC__)
/// @brief Aligns the current data type with the given number of bytes.
/// @param alignment The alignment of the current structure.
#define GSIM_ALIGNAS(alignment) __attribute__((aligned(alignment)))
#elif defined(_MSC_VER)
/// @brief Aligns the current data type with the given number of bytes.
/// @param alignment The alignment of the current structure.
#define GSIM_ALIGNAS(alignment) __declspec(align(alignment))
#endif