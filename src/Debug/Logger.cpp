#include "Logger.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace gsim {
	// Constants
	static const size_t MAX_MESSAGE_LENGTH = 16384;

	static const char* const LOG_LEVEL_NAMES[] = {
		"[FATAL]:   ",
		"[ERROR]:   ",
		"[WARNING]: ",
		"[INFO]:    ",
		"[DEBUG]:   "
	};
	static const size_t LOG_LEVEL_NAME_LENGTH = 11;

	// Internal variables
	static FILE* logFile;

	void CreateLogger(const char* logFilePath) {
		// Open the log file
		logFile = fopen(logFilePath, "w");

		if(!logFile)
			GSIM_LOG_FATAL("Failed to open log file!");
	}
	void DestroyLogger() {
		// Close the log file
		fclose(logFile);
	}

	void LogMessage(LogLevel level, const char* format, ...) {
		// Get the vague arguments list
		va_list args;
		va_start(args, format);

		// Format the message
		char message[LOG_LEVEL_NAME_LENGTH + MAX_MESSAGE_LENGTH + 1];
		memcpy(message, LOG_LEVEL_NAMES[level], LOG_LEVEL_NAME_LENGTH);
		vsnprintf(message + LOG_LEVEL_NAME_LENGTH, MAX_MESSAGE_LENGTH, format, args);

		size_t formattedLen = strnlen(message, LOG_LEVEL_NAME_LENGTH + MAX_MESSAGE_LENGTH);
		message[formattedLen] = '\n';

		// Output the message to the console and the file
		if(level == LOG_LEVEL_ERROR || level == LOG_LEVEL_FATAL)
			fwrite(message, 1, formattedLen + 1, stderr);
		else
			fwrite(message, 1, formattedLen + 1, stdout);
		if(logFile)
			fwrite(message, 1, formattedLen + 1, logFile);

		// End the vague arguments list
		va_end(args);

		// Exit the program if the current level is a fatal error
		if(level == LOG_LEVEL_FATAL)
			abort();
	}
}