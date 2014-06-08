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

#include <array>
#include <set>
#include "Entity.h"
#include <Core/TMPUtils.h>

namespace spades { namespace game {
	
	class PlayerEntityListener;
	
	class PlayerEntity: public Entity {
		PlayerInput playerInput;
		ToolSlot tool;
		IntVector3 blockColor;
		
		std::set<PlayerEntityListener *> listeners;
		
		std::array<std::string, 3> weaponSkins;
		std::string bodySkin;
		
		
		
		virtual void EvaluateTrajectory(Duration);
		
	public:
		PlayerEntity(World&);
		~PlayerEntity();
		
		using Entity::AddListener;
		using Entity::RemoveListener;
		void AddListener(PlayerEntityListener *l);
		void RemoveListener(PlayerEntityListener *l);
		
		float GetHeight(bool crouch);
		float GetBoxSize();
		
		std::string GetName() override;
		
		/** Updates the local state of the player. */
		void SetPlayerInput(const PlayerInput&);
		/** Updates the local and remote state of the player. */
		void UpdatePlayerInput(const PlayerInput&);
		const PlayerInput& GetPlayerInput() const { return playerInput; }
		
		float GetJumpVelocity();
		bool IsOnGroundOrWade();
		
		
		void EventTriggered(EntityEventType,
							uint64_t param) override;
		
		void Accept(EntityVisitor& v) override { v.Visit(*this); }
	};
	
	class PlayerEntityListener {
	public:
		virtual ~PlayerEntityListener() { }
		
		/** Called when a player jumped. Intended to be handled by
		 * client routine. */
		virtual void Jumped(PlayerEntity&) { }
		
		/** Called when player state was modified, and should be
		 * sent to the remote peer. */
		virtual void PlayerInputUpdated(PlayerEntity&) { }
	};
	
} }

