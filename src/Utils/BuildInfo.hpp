#pragma once

// Windows
#if defined(WIN32) | defined(_WIN32) | defined(__WIN32__)
/// @brief Defined when building for Windows.
#define GSIM_PLATFORM_WINDOWS
#endif

// Linux
#if defined(__linux__) || defined(__gnu_linux__)
/// @brief Defined when building for Linux.
#define GSIM_PLATFORM_LINUX
#endif

// Check if the target platform is supported
#if !defined(GSIM_PLATFORM_WINDOWS) && !defined(GSIM_PLATFORM_LINUX)
// Throw an error
#error "Unsupported target platform!"
#endif

// Architecture
#if defined(__i386__)
// Defined when building for a 32-bit architecture
#define GSIM_ARCHITECTURE_X86_64
#else
// Defined when building for a 64-bit architecture
#define GSIM_ARCHITECTURE_X64
#endif

// Build mode
#if !defined(NDEBUG)
// Defined when building in debug mode
#define GSIM_BUILD_MODE_DEBUG
#else
// Defined when building in release mode
#define GSIM_BUILD_MODE_RELEASE
#endif