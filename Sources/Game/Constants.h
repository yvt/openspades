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
#include <Core/TMPUtils.h>

namespace spades { namespace game {
	
	enum class EntityType {
		Player,
		Grenade,
		Rocket,
		CommandPost,
		Flag,
		Checkpoint,
		Vehicle
	};
	
	enum class EntityEventType {
		// player
		Jump,
		ReloadWeapon,
		Suicide,
		
		// common
		Explode
	};
	
	enum class EntityDeathType {
		Unspecified = 0,
		PlayerDeath,
		Explode
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
		
		/** When parentEntityId is set, the entity is locked in the 
		 * local coordinate space of the parent entity. */
		stmp::optional<std::uint32_t> parentEntityId;
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
	
	enum class HitType {
		Unspecified = 0,
		Torso,
		Head,
		Limb
	};
	
	enum class DamageType {
		Unknown,
		Suicide,
		Fall,
		Poison,
		
		Melee,
		Dig,
		Block,
		Weapon1,
		Weapon2,
		Weapon3,
		
		Grenade,
		
		VehicleExplosion,
		Vehicle
	};
	
	struct DamageInfo {
		stmp::optional<uint32_t> fromEntity;
		uint32_t toEntity;
		
		DamageType damageType;
		
		Vector3 firePosition;
		Vector3 hitPosition;
	};
	
	using SkinId = std::array<char, 20>;
	
} }

