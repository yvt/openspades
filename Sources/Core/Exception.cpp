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

#include "Exception.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

namespace spades {
	static char buf[65536];
	Exception::Exception(const char *format, ...) {
		va_list va;
		va_start(va, format);
		vsprintf(buf, format, va);
		va_end(va);
		message = buf;
	}
	Exception::Exception(const char *file, int line, const char *format, ...) {
		reflection::Backtrace& trace =
		*reflection::Backtrace::GetGlobalBacktrace();
		
		va_list va;
		va_start(va, format);
		vsprintf(buf, format, va);
		va_end(va);
		message = buf;
		shortMessage = message;
		
		sprintf(buf, "[%s:%d] %s", file, line, message.c_str());
		message = buf;
		
		
		message += "\nCollected backtrace:\n";
		message += trace.ToString();
	}
	Exception::~Exception() throw(){}
	const char *Exception::what() const throw() {
		return message.c_str();
	}
	
}
