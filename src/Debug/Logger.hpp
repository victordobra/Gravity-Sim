#pragma once

#include <stdint.h>
#include "Utils/BuildInfo.hpp"

namespace gsim {
    /// @brief The danger level of a log message.
    typedef enum {
		/// @brief Used to report a fatal error which instantly stops the program.
		LOG_LEVEL_FATAL,
		/// @brief Used to report an error which is causing incorrect, yet non-fatal behaviour.
		LOG_LEVEL_ERROR,
		/// @brief Used to report unoptimal behaviour that doesn't cause the program to behave incorrectly.
		LOG_LEVEL_WARNING,
		/// @brief Used to log information that might be relevant to the user.
		LOG_LEVEL_INFO,
		/// @brief Used for debugging only. Not enabled during release by default.
		LOG_LEVEL_DEBUG
	} LogLevel;

	/// @brief Creates the debug logger.
	/// @param logFilePath The path to the file in which all logged messages will be written
	void CreateLogger(const char* logFilePath);
	/// @brief Destroys the debug logger.
	void DestroyLogger();

	/// @brief Logs a message.
	/// @param level The message level.
	/// @param format The message string format.
	void LogMessage(LogLevel level, const char* format, ...);

/// @brief Defined when warnings are enabled for the logger.
#define GSIM_LOG_WARNING_ENABLED
/// @brief Defined when info messages are enabled for the logger.
#define GSIM_LOG_INFO_ENABLED

#if defined(GSIM_BUILD_MODE_DEBUG)
/// @brief Defined when debug messages are enabled for the logger.
#define GSIM_LOG_DEBUG_ENABLED
#endif

/// @brief Logs a fatal error.
/// @param format The message string format.
#define GSIM_LOG_FATAL(format, ...) gsim::LogMessage(gsim::LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
/// @brief Logs an error.
/// @param format The message string format.
#define GSIM_LOG_ERROR(format, ...) gsim::LogMessage(gsim::LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#ifdef GSIM_LOG_WARNING_ENABLED
/// @brief Logs a warning.
/// @param format The message string format.
#define GSIM_LOG_WARNING(format, ...) gsim::LogMessage(gsim::LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#else
/// @brief Logs a warning.
/// @param format The message string format.
#define GSIM_LOG_WARNING(format, ...)
#endif

#ifdef GSIM_LOG_INFO_ENABLED
/// @brief Logs an info message.
/// @param format The message string format.
#define GSIM_LOG_INFO(format, ...) gsim::LogMessage(gsim::LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#else
/// @brief Logs an info message.
/// @param format The message string format.
#define GSIM_LOG_INFO(format, ...)
#endif

#ifdef GSIM_LOG_DEBUG_ENABLED
/// @brief Logs a debug message.
/// @param format The message string format.
#define GSIM_LOG_DEBUG(format, ...) gsim::LogMessage(gsim::LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#else
/// @brief Logs a debug message.
/// @param format The message string format.
#define GSIM_LOG_DEBUG(format, ...)
#endif
}