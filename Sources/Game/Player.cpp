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

#include "Player.h"
#include <Core/Debug.h>
#include "PlayerEntity.h"
#include "world.h"

namespace spades { namespace game {
	
	Player::Player(World& world,
				   const std::string& name):
	world(world),
	name(name) {
		SPADES_MARK_FUNCTION();
		
		flags.isAdmin = false;
	}
	
	Player::~Player() {
		SPADES_MARK_FUNCTION();
	}
	
	void Player::AddListener(PlayerListener *l) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(l);
		listeners.insert(l);
	}
	
	void Player::RemoveListener(PlayerListener *l) {
		listeners.erase(l);
	}
	
	void Player::SetId(stmp::optional<uint32_t> id) {
		SPADES_MARK_FUNCTION();
		
		if (playerId && id) {
			// reassignment is not allowed
			SPAssert(false);
		}
		if (!id) {
			if (GetWorld().IsLocalHostServer()) {
				// also have to remove entity
				auto *e = GetEntity();
				if (e) {
					GetWorld().UnlinkEntity(e);
				}
			}
		}
		playerId = id;
		if (!playerId) {
			for (auto *l: listeners)
				l->PlayerRemoved(*this);
		}
	}
	
	PlayerEntity *Player::GetEntity() {
		SPADES_MARK_FUNCTION();
		
		auto *e = world.FindEntity(*playerId);
		if (!e) return nullptr;
		auto *pe = dynamic_cast<PlayerEntity *>(e);
		SPAssert(pe);
		return pe;
	}
	
	void Player::Spawn() {
		SPADES_MARK_FUNCTION();
		
		SPAssert(GetWorld().IsLocalHostServer());
		
		if (GetEntity()) {
			GetWorld().UnlinkEntity(GetEntity());
		}
		
		auto *pe = new PlayerEntity(GetWorld());
		GetWorld().LinkEntity(pe, *GetId());
		pe->Release();
		SPAssert(GetEntity());
	}
	
} }
