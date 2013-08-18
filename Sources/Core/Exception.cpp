//
//  Exception.cpp
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
