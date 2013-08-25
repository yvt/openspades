//
//  Stopwatch.cpp
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Stopwatch.h"
#include "../Imports/SDL.h"
#include "Debug.h"

#ifdef WIN32
#include <windows.h>
static double GetSWTicks() {
	LARGE_INTEGER val, freq;
	if(QueryPerformanceFrequency(&freq) == 0 || freq.QuadPart == 0 ||
	   QueryPerformanceCounter(&val) == 0 || val.QuadPart == 0){
		return (uint64_t)GetTickCount() / 1000.;
	}
	return (double)val.QuadPart / (double)freq.QuadPart;
}
#else
#include <sys/time.h>
static double GetSWTicks() {
	struct timeval val;
	gettimeofday(&val, NULL);
	return (double)val.tv_usec / 1000000. +
	(double)val.tv_sec;
}
#endif

namespace  spades {
	Stopwatch::Stopwatch(){
		Reset();
	}
	void Stopwatch::Reset() {
		start = GetSWTicks();
	}
	double Stopwatch::GetTime() {
		return GetSWTicks() - start;
	}
}
