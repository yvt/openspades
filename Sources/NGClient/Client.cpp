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

#include "Client.h"

#include "Arena.h"
#include <Game/World.h>

namespace spades { namespace ngclient {
	
	Client::Client(client::IRenderer *renderer,
				   client::IAudioDevice *audio,
				   const ClientParams& params):
	renderer(renderer),
	audio(audio),
	params(params) {
		SPAssert(renderer);
		SPAssert(audio);
		
		renderer->SetGameMap(nullptr);
		
		arena.Set(new Arena(this), false);
		arena->SetupRenderer();
	}
	
	Client::~Client() {
		arena->UnsetupRenderer();
		arena.Set(nullptr);
	}
	
	void Client::RunFrame(float dt) {
		if (arena) {
			arena->RunFrame(dt);
		}
		
		renderer->Flip();
	}
	
	void Client::Closing() {
		arena.Set(nullptr);
	}
	
	bool Client::WantsToBeClosed() {
		return false;
	}
	
	Client::InputRoute Client::GetInputRoute() {
		return InputRoute::Arena;
	}
	
	
} }

