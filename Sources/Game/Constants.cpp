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

#include "Constants.h"
#include <Core/Strings.h>
#include <Core/Debug.h>

namespace spades { namespace game {
	
	std::string GetEntityTypeName(EntityType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case EntityType::Player:
				return "Player";
			case EntityType::Grenade:
				return "Grenade";
			case EntityType::Rocket:
				return "Rocket";
			case EntityType::CommandPost:
				return "CommandPost";
			case EntityType::Flag:
				return "Flag";
			case EntityType::Checkpoint:
				return "Checkpoint";
			case EntityType::Vehicle:
				return "Vehicle";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string GetEntityEventTypeName(EntityEventType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case EntityEventType::Jump:
				return "Jump";
			case EntityEventType::ReloadWeapon:
				return "ReloadWeapon";
			case EntityEventType::Suicide:
				return "Suicide";
			case EntityEventType::Explode:
				return "Explode";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string GetEntityDeathTypeName(EntityDeathType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case EntityDeathType::Unspecified:
				return "Unspecified";
			case EntityDeathType::PlayerDeath:
				return "PlayerDeath";
			case EntityDeathType::Explode:
				return "Explode";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string GetBlockCreateTypeName(BlockCreateType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case BlockCreateType::Unspecified:
				return "Unspecified";
			case BlockCreateType::Player:
				return "Player";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string GetBlockDestroyTypeName(BlockDestroyType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case BlockDestroyType::Unspecified:
				return "Unspecified";
			case BlockDestroyType::Damage:
				return "Damage";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string GetWeaponTypeName(WeaponType t) {
		SPADES_MARK_FUNCTION();
		
		switch (t) {
			case WeaponType::Bullet:
				return "Bullet";
			case WeaponType::Shotgun:
				return "Shotgun";
			case WeaponType::Rocket:
				return "Rocket";
		}
		return Format("Unknown ({0})",
					  static_cast<int>(t));
	}
	
	std::string WeaponParameters::ToString() {
		return Format("{0}, {1}, {2}, {3}, "
					  "interval = {4} [ms], "
					  "damage = {5} [%], "
					  "reload time = {6} [ms], "
					  "magazine size = {7} [rounds],"
					  "raise time = {8} [ms]",
					  isFullAutomatic ? "Full-auto" : "Semi-auto",
					  doesReloadsSlow ? "Slow Reload" : "Magazine",
					  penetrative ? "Penetrative" : "Not Penetrative",
					  GetWeaponTypeName(type),
					  fireInterval,
					  maxDamage,
					  reloadTime,
					  magazineSize,
					  raiseTime);
	}
} }
