//
//  Mutex.cpp
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Mutex.h"
#include "../Imports/SDL.h"

namespace spades {
	Mutex::Mutex() {
		priv = (void *)SDL_CreateMutex();
	}
	
	Mutex::~Mutex() {
		SDL_DestroyMutex((SDL_mutex *)priv);
	}
	
	void Mutex::Lock(){
		SDL_mutexP((SDL_mutex *)priv);
	}
	
	void Mutex::Unlock(){
		SDL_mutexV((SDL_mutex *)priv);
	}
}
