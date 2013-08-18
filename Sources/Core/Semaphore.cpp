//
//  Semaphore.cpp
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Semaphore.h"
#include "../Imports/SDL.h"

namespace spades {
	Semaphore::Semaphore(int initial) {
		priv = (void *)SDL_CreateSemaphore(initial);
	}
	
	Semaphore::~Semaphore() {
		SDL_DestroySemaphore((SDL_sem *)priv);
	}
	
	void Semaphore::Post(){
		SDL_SemPost((SDL_sem *)priv);
	}
	
	void Semaphore::Wait(){
		SDL_SemWait((SDL_sem *)priv);
	}
}
