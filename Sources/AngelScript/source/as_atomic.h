/*
   AngelCode Scripting Library
   Copyright (c) 2003-2013 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_atomic.h
//
// The asCAtomic class provides methods for performing threadsafe
// operations on a single dword, e.g. reference counting and 
// bitfields.
//



#ifndef AS_ATOMIC_H
#define AS_ATOMIC_H

#include <atomic>
#include "as_config.h"

BEGIN_AS_NAMESPACE

class asCAtomic
{
public:
	asCAtomic() {}

	asDWORD get() const { return value.load(); }
	void    set(asDWORD val) { value.store(val); }

	// Increase and return new value
	asDWORD atomicInc() { return value.fetch_add(1) + 1; }

	// Decrease and return new value
	asDWORD atomicDec() { return value.fetch_sub(1) - 1; }

protected:
	std::atomic<asDWORD> value {0};
};

END_AS_NAMESPACE

#endif
