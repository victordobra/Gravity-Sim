#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

namespace gsim {
	/// @brief A class that representa ax exception thrown by the program.
	class Exception {
	public:
		Exception() = delete;
		/// @brief Copies the given extension.
		/// @param other The extension to copy.
		Exception(const Exception& other) = default;
		/// @brief Moves the given exception's contents.
		/// @param other The exception to move.
		Exception(Exception&& other) noexcept = default;
		/// @brief Creates an exception with the given info.
		/// @param file The path of the file in which the exception was thrown.
		/// @param line The line in the source file at which the exception was thrown.
		/// @param format The format to use for the exception's message.
		Exception(const char* file, uint32_t line, const char* format, ...) : file(file), line(line) {
			// Get the va list
			va_list args;
			va_start(args, format);

			// Format the message
			vsnprintf(message, MAX_MESSAGE_LEN, format, args);

			// End the va list
			va_end(args);
		}

		/// @brief Copies the given exception's contents into this exception.
		/// @param other The exception to copy.
		/// @return A reference to this exception.
		Exception& operator=(const Exception& other) = default;
		/// @brief Moves the given exception's contents into this exception.
		/// @param other The exception to move.
		/// @return A reference to this exception.
		Exception& operator=(Exception&& other) = default;

		/// @brief Gets the path of the file from which the exception was thrown.
		/// @return The path of the file from which the exception was thrown.
		const char* GetFile() const {
			return file;
		}
		/// @brief Gets the line in the source file at which the exception was thrown.
		/// @return The line in the source file at which the exception was thrown.
		uint32_t GetLine() const {
			return line;
		}
		/// @brief Gets the exception's message.
		/// @return The exception's message.
		const char* GetMessage() const {
			return message;
		}

		/// @brief Destroys the exception.
		~Exception() = default;
	private:
		static const size_t MAX_MESSAGE_LEN = 256;

		const char* file;
		uint32_t line;
		char message[MAX_MESSAGE_LEN];
	};

/// @brief Throws an exception with info on the current file and line.
/// @param format The format to use for the exception's message.
#define GSIM_THROW_EXCEPTION(format, ...) throw gsim::Exception(__FILE__, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
}