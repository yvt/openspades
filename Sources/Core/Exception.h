/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <exception>
#include <string>

#include "Debug.h"

namespace spades {
	class Exception : public std::exception {
		std::string shortMessage;
		std::string message;

	public:
		Exception(const char *format, ...);
		Exception(const char *file, int line, const char *format, ...);
		~Exception() throw();
		const char *what() const throw() override;
		const std::string &GetShortMessage() const throw() { return shortMessage; }
	};
}

#ifdef _MSC_VER
#define SPRaise(fmt, ...) throw ::spades::Exception(__FILE__, __LINE__, fmt, __VA_ARGS__)
#else
#define SPRaise(fmt, val...) throw ::spades::Exception(__FILE__, __LINE__, fmt, ##val)
#endif

/**
 * Raises a "not implemented" exception.
 *
 * This serves as a dynamic "TODO" marker. Mere occurrences of this macro don't mean there are
 * missing features. They have to be *reachable*.
 *
 * Do not use this to indicate unreachable code! Use `SPUnreachable` instead in such cases.
 */
#define SPNotImplemented() SPRaise("Not implemented")

#define SPUnsupported() SPRaise("Unsupported")

#define SPUnreachable() SPRaise("Internal error; unreachable code")

#define SPInvalidArgument(name) SPRaise("Invalid argument: %s", name)

#define SPInvalidEnum(name, value) SPRaise("Invalid enum: %s: %d", name, (int)(value))

#define SPFileNotFound(fn) SPRaise("File not found: %s", fn)
