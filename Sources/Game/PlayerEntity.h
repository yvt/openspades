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
	
	struct PlayerWeapon {
		std::string skin;
		WeaponParameters param;
	};
	
	class PlayerEntity: public Entity {
		PlayerInput playerInput;
		ToolSlot tool;
		IntVector3 blockColor;
		
		float moveDistance = 0.f;
		int moveSteps = 0;
		bool airborne = false;
		
		std::set<PlayerEntityListener *> listeners;
		
		std::array<PlayerWeapon, 3> weapons;
		std::string bodySkin;
		
		
		virtual void EvaluateTrajectory(Duration);
		
		
		void BoxClipMove(Duration);
		void WalkMove(Duration);
		
		void FlyMove(Duration);
		
	public:
		PlayerEntity(World&);
		~PlayerEntity();
		
		using Entity::AddListener;
		using Entity::RemoveListener;
		void AddListener(PlayerEntityListener *l);
		void RemoveListener(PlayerEntityListener *l);
		
		float GetHeight(PlayerStance);
		float GetCurrentHeight();
		float GetBoxSize();
		
		const PlayerWeapon& GetWeapon(int);
		void SetWeapon(int, const PlayerWeapon&);
		
		std::string GetBodySkin() { return bodySkin; }
		void SetBodySkin(const std::string&);
		
		std::string GetName() override;
		bool IsLocallyControlled() override;
		
		/** Updates the local state of the player. */
		void SetPlayerInput(const PlayerInput&);
		/** Updates the local and remote state of the player. */
		void UpdatePlayerInput(const PlayerInput&);
		bool FixInput(PlayerInput&);
		const PlayerInput& GetPlayerInput() const { return playerInput; }
		ToolSlot GetTool() const { return tool; }
		IntVector3 GetBlockColor() const { return blockColor; }
		
		void SetTool(ToolSlot s) { tool = s;}
		void SetBlockColor(const IntVector3& c) { blockColor = c; }
		
		float GetJumpVelocity();
		bool IsOnGroundOrWade();
		bool IsWading();
		bool IsCurrentToolWeapon();
		
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
		
		virtual void Footstep(PlayerEntity&) { }
		
		virtual void Fell(PlayerEntity&, bool hurt) { }
	};
	
} }

