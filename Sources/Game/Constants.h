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

#pragma once

#include <Core/Math.h>
#include <cstdint>
#include <array>

namespace spades { namespace game {
	
	enum class EntityType {
		Player,
		Grenade,
		Rocket,
		CommandPost,
		Checkpoint,
		Vehicle
	};
	
	struct EntityFlags {
		bool playerClip : 1;
		bool weaponClip : 1;
		bool fly : 1;
	};
	
	enum class TrajectoryType {
		Constant,
		Linear,
		Gravity,
		Player,
		RigidBody
	};
	
	enum class ToolSlot {
		Melee,
		Block,
		Weapon1,
		Weapon2,
		Weapon3
	};
	
	struct Trajectory {
		TrajectoryType type;
		Vector3 origin;
		Vector3 velocity;
		Quaternion angle; // used except for player
		Vector3 angularVelocity; // used except for player
		Vector3 eulerAngle; // used only for player
	};
	
	enum class PlayerStance {
		Standing,
		Crouch,
		Prone
	};
	
	struct PlayerInput {
		std::int8_t xmove;
		std::int8_t ymove;
		PlayerStance stance : 2;
		bool toolPrimary : 1;
		bool toolSecondary : 1;
		bool chat : 1;
		bool sprint : 1;
	};
	
	using SkinId = std::array<char, 20>;
	
} }

