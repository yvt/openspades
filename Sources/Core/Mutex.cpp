/*
 Copyright (c) 2013 OpenSpades Developers
 
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
