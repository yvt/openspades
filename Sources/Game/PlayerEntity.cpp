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

#include "PlayerEntity.h"
#include <Core/Strings.h>
#include "World.h"

namespace spades { namespace game {
	PlayerEntity::PlayerEntity(World& world):
	Entity(world, EntityType::Player),
	tool(ToolSlot::Weapon1),
	blockColor(192, 192, 192) {
		
	}
	PlayerEntity::~PlayerEntity() {
		
	}
	
	void PlayerEntity::AddListener(PlayerEntityListener *l) {
		listeners.insert(l);
	}
	void PlayerEntity::RemoveListener(PlayerEntityListener *l) {
		listeners.erase(l);
	}
	
	float PlayerEntity::GetBoxSize() {
		return 0.45f;
	}
	
	float PlayerEntity::GetHeight(bool crouch) {
		return crouch ? 1.5f : 2.5f;
	}
	
	std::string PlayerEntity::GetName() {
		// TODO: PlayerEntity::GetName
		return Format("{0} \"{1}\"", Entity::GetName(), "[player name here]");
	}
	
	void PlayerEntity::SetPlayerInput(const PlayerInput& input) {
		playerInput = input;
		
	}
	
	void PlayerEntity::UpdatePlayerInput(const PlayerInput& input) {
		SetPlayerInput(input);
		for (auto *listener: listeners) listener->PlayerInputUpdated(*this);
	}
	
#pragma mark - Player Movement
	
	float PlayerEntity::GetJumpVelocity() {
		return GetWorld().GetParameters().playerJumpVelocity;
	}
	
	void PlayerEntity::EvaluateTrajectory(Duration dt) {
		SPAssert(GetTrajectory().type == TrajectoryType::Player);
		
		// TODO: EvaluateTrajectory
	}
	
	bool PlayerEntity::IsOnGroundOrWade() {
		// TODO: IsOnGroundOrWade
		return true;
	}
	
	void PlayerEntity::EventTriggered(EntityEventType type, uint64_t param) {
		if (type == EntityEventType::Jump) {
			for (auto *listener: listeners) listener->Jumped(*this);
			if (IsOnGroundOrWade()) {
				GetTrajectory().velocity.z -= 0.36f;
			}
		} else {
			Entity::EventTriggered(type, param);
		}
	}
	
} }
