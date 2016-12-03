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

#include <Imports/SDL.h>

#include "Stopwatch.h"
#include "Debug.h"

#ifdef WIN32
#include <windows.h>
static double GetSWTicks() {
	LARGE_INTEGER val, freq;
	if (QueryPerformanceFrequency(&freq) == 0 || freq.QuadPart == 0 ||
	    QueryPerformanceCounter(&val) == 0 || val.QuadPart == 0) {
		return (uint64_t)GetTickCount() / 1000.;
	}
	return (double)val.QuadPart / (double)freq.QuadPart;
}
#else
#include <sys/time.h>
static double GetSWTicks() {
	struct timeval val;
	gettimeofday(&val, NULL);
	return (double)val.tv_usec / 1000000. + (double)val.tv_sec;
}
#endif

namespace spades {
	Stopwatch::Stopwatch() { Reset(); }
	void Stopwatch::Reset() { start = GetSWTicks(); }
	double Stopwatch::GetTime() { return GetSWTicks() - start; }
}
