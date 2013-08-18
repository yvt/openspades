//
//  Exception.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include <exception>
#include "Debug.h"

namespace spades {
	class Exception: public std::exception {
		std::string message;
	public:
		Exception(const char *format, ...);
		Exception(const char *file, int line, const char *format, ...);
		virtual ~Exception() throw();
		virtual const char *what() const throw();
	};
}

#define SPRaise(fmt, val...) throw ::spades::Exception(__FILE__, __LINE__, fmt, ##val )

#define SPNotImplemented() SPRaise("Not implemented")

#define SPInvalidArgument(name) SPRaise("Invalid argument: %s", name)

#define SPInvalidEnum(name, value) SPRaise("Invalid enum: %s: %d", name, (int)(value))

#define SPFileNotFound(fn) SPRaise("File not found: %s", fn)
