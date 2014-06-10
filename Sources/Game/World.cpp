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

#include "World.h"

namespace spades { namespace game {
	
	WorldParameters::WorldParameters():
	playerJumpVelocity(0.36f),
	fallDamageVelocity(0.58f),
	fatalFallDamageVelocity(1.f) { }
	
	World::World():
	currentTime(0) {
		SPNotImplemented();
	}
	
	World::~World() {
		
	}
	
	void World::Advance(Duration) {
		SPNotImplemented();
	}
	
	Vector3 World::GetGravity()	{
		return Vector3(0.f, 0.f, 10.f); // FIXME: correct value
	}
	
	bool World::IsLocalHostServer() {
		return false; // TODO: IsLocalHostServer
	}
	
	bool World::IsWaterAt(const Vector3& v) {
		return v.z > 63.f;
	}
	
	stmp::optional<uint32_t> World::GetLocalPlayerId() {
		return stmp::optional<uint32_t>();
	}
	
} }

