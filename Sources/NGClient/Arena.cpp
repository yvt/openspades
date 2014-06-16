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

#include "Arena.h"
#include "Client.h"
#include <Core/Debug.h>
#include <Game/World.h>
#include <Core/IStream.h>
#include <Core/FileManager.h>

#include "LocalEntity.h"
#include "PlayerLocalEntity.h"

namespace spades { namespace ngclient {
	
	Arena::Arena(Client *client, game::World *world):
	client(client),
	renderer(client->renderer),
	audio(client->audio),
	world(world){
		SPADES_MARK_FUNCTION();
		
		SPAssert(renderer);
		SPAssert(audio);
		
		world->AddListener(this);
		
		LoadEntities();
	}
	
	Arena::~Arena() {
		SPADES_MARK_FUNCTION();
		
		localEntities.clear();
		
		world->RemoveListener(this);
		world.Set(nullptr);
	}
	
	void Arena::Initialize() {
		SPADES_MARK_FUNCTION();
		
		renderer->Init();
		
		playerLocalEntityFactory.reset
		(new PlayerLocalEntityFactory(*this));
	}
	
	void Arena::RunFrame(float _dt) {
		SPADES_MARK_FUNCTION();
		game::Duration dt = _dt;
		
		world->Advance(dt);
		
		{
			auto it = localEntities.begin();
			while (it != localEntities.end()) {
				auto cur = it++;
				const auto& lEnt = *cur;
				if (!lEnt->Update(dt)) {
					localEntities.erase(cur);
				}
			}
		}
		
		Render();
		
		time += std::min(dt, .1);
	}
	
	bool Arena::WantsToBeClosed() {
		SPADES_MARK_FUNCTION();
		return false;
	}
	
	
} }

