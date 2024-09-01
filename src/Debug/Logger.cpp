#include "Logger.hpp"
#include <stdio.h>
#include <stdlib.h>

namespace gsim {
	// Constants
	static const size_t MAX_MESSAGE_LEN = 2048;

	// Internal functions
	static const char* MessageLevelToString(Logger::MessageLevel messageLevel) {
		switch(messageLevel) {
		case Logger::MESSAGE_LEVEL_DEBUG:
			return "[DEBUG]       ";
		case Logger::MESSAGE_LEVEL_INFO:
			return "[INFO]        ";
		case Logger::MESSAGE_LEVEL_WARNING:
			return "[WARNING]     ";
		case Logger::MESSAGE_LEVEL_ERROR:
			return "[ERROR]       ";
		case Logger::MESSAGE_LEVEL_FATAL_ERROR:
			return "[FATAL ERROR] ";
		default:
			return "[LOG]         ";
		}
	}

	// Public functions
	Logger::Logger(const char* filePath, MessageLevelFlags messageLevelFlags) : levelFlags(messageLevelFlags) {
		// Check if a log file will be used
		if(filePath) {
			// Try to create the log file
			logFile = fopen(filePath, "w");
			if(!logFile)
				LogMessage(MESSAGE_LEVEL_ERROR, "Failed to open log file! All log messages will be outputted only to the console.");
		} else {
			logFile = nullptr;
		}
	}

	void Logger::LogMessage(MessageLevel level, const char* format, ...) {
		// Exit the function if the message level is not considered by the logger
		if(!(level & levelFlags))
			return;
		
		// Get the va list
		va_list args;
		va_start(args, format);

		// Format the message
		char message[MAX_MESSAGE_LEN];
		vsnprintf(message, MAX_MESSAGE_LEN, format, args);

		// End the va list
		va_end(args);

		// Get the message level's string
		const char* levelString = MessageLevelToString(level);

		// Output the message to the log file, if it exists
		if(logFile) {
			fputs(levelString, logFile);
			fputs(message, logFile);
			fputc('\n', logFile);
			fflush(logFile);
		}

		// Output the message to the console
		fputs(levelString, stdout);
		fputs(message, stdout);
		fputc('\n', stdout);
		fflush(stdout);

		// Close the program if the messsage is a fatal error
		if(level == MESSAGE_LEVEL_FATAL_ERROR)
			abort();
	}
	void Logger::LogException(const Exception& exception) {
		// Log a message containing the exception's info
		LogMessage(MESSAGE_LEVEL_FATAL_ERROR, "Exception thrown at file \"%s\", line %u: \"%s\"", exception.GetFile(), exception.GetLine(), exception.GetMessage());
	}
	void Logger::LogStdException(const std::exception& exception) {
		// Log a message containing the exception's info
		LogMessage(MESSAGE_LEVEL_FATAL_ERROR, "Standard library exception thrown: \"%s\"", exception.what());
	}

	Logger::~Logger() {
		// Close the log file stream, if it exists
		if(logFile) {
			fclose(logFile);
		}
	}
}