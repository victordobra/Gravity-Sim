#pragma once

#include <stdint.h>
#include <exception>
#include "Exception.hpp"

namespace gsim {
	class Logger {
	public:
		/// @brief An enum containing all message levels.
		enum MessageLevel {
			/// @brief The level of a debug message, useful for debugging.
			MESSAGE_LEVEL_DEBUG = 0x01,
			/// @brief The level of an info message that may give the user important information.
			MESSAGE_LEVEL_INFO = 0x02,
			/// @brief The level of a warning message that may indicate incorrect program behaviour.
			MESSAGE_LEVEL_WARNING = 0x04,
			/// @brief The level of an error message that indicates incorrect program behaviour.
			MESSAGE_LEVEL_ERROR = 0x08,
			/// @brief The level of a faltal error message that instantly closes the program.
			MESSAGE_LEVEL_FATAL_ERROR = 0x10,
			/// @brief A bitmask containing the flags of essential message levels.
			MESSAGE_LEVEL_ESSENTIAL = MESSAGE_LEVEL_WARNING | MESSAGE_LEVEL_ERROR | MESSAGE_LEVEL_FATAL_ERROR,
			/// @brief A bitmask containing the flags of all message levels.
			MESSAGE_LEVEL_ALL = MESSAGE_LEVEL_DEBUG | MESSAGE_LEVEL_INFO | MESSAGE_LEVEL_WARNING | MESSAGE_LEVEL_ERROR | MESSAGE_LEVEL_FATAL_ERROR
		};
		/// @brief The type of a bitmask containing zero or more message level flags.
		typedef uint32_t MessageLevelFlags;

		Logger() = delete;
		Logger(const Logger&) = delete;
		Logger(Logger&&) noexcept = delete;
		/// @brief Creates a debug logger.
		/// @param filePath The path of the log output file, or nullptr if no log file will be used.
		/// @param messageLevelFlags A bitmask of the message level flags which will be parsed by the logger.
		Logger(const char* filePath, MessageLevelFlags messageLevelFlags);

		Logger& operator=(const Logger&) = delete;
		Logger& operator=(Logger&&) = delete;

		/// @brief Logs a message.
		/// @param level The message's level.
		/// @param format The format to use for the message.
		void LogMessage(MessageLevel level, const char* format, ...);
		/// @brief Logs a message, without comparing its level with the logger's flags.
		/// @param level The message's level.
		/// @param format The format to use for the message.
		void LogMessageForced(MessageLevel level, const char* format, ...);
		/// @brief Logs an exception.
		/// @param exception The exception to log.
		void LogException(const Exception& exception);
		/// @brief Logs a standard library exception.
		/// @param exception The exception to log.
		void LogStdException(const std::exception& exception);

		/// @brief Destroys the debug logger.
		~Logger();
	private:
		void LogMessageInternal(MessageLevel level, const char* message);

		FILE* logFile;
		MessageLevelFlags levelFlags;
	};
}