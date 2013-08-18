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

namespace  spades {
	Stopwatch::Stopwatch(){
		Reset();
	}
	void Stopwatch::Reset() {
		start = SDL_GetTicks();
	}
	unsigned int Stopwatch::GetTime() {
		return SDL_GetTicks() - start;
	}
}
