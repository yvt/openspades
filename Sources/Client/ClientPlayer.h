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

#include <Core/RefCountedObject.h>
#include <ScriptBindings/ScriptManager.h>
#include <Core/Math.h>
#include "Player.h"

namespace spades {
	namespace client {
		
		class Client;
		
		/** Representation of player which is used by
		 * drawing/view layer of game client. */
		class ClientPlayer: public RefCountedObject {
			Client *client;
			Player *player;
			
			float sprintState;
			float aimDownState;
			float toolRaiseState;
			Player::ToolType currentTool;
			float localFireVibrationTime;
			float time;
			
			Vector3 viewWeaponOffset;
			
			asIScriptObject *spadeSkin;
			asIScriptObject *blockSkin;
			asIScriptObject *weaponSkin;
			asIScriptObject *grenadeSkin;
			
			asIScriptObject *spadeViewSkin;
			asIScriptObject *blockViewSkin;
			asIScriptObject *weaponViewSkin;
			asIScriptObject *grenadeViewSkin;
			
			Matrix4 GetEyeMatrix();
			void AddToSceneThirdPersonView();
			void AddToSceneFirstPersonView();
			
			void SetSkinParameterForTool(Player::ToolType,
										 asIScriptObject *);
			void SetCommonSkinParameter(asIScriptObject *);
			
			float GetLocalFireVibration();
			
			bool ShouldRenderInThirdPersonView();
		protected:
			virtual ~ClientPlayer();
		public:
			ClientPlayer(Player *p, Client *);
			Player *GetPlayer() const { return player; }
			
			void Invalidate();
			
			void Update(float dt);
			void AddToScene();
			void Draw2D();
			
			bool IsChangingTool();
			void FiredWeapon();
			void ReloadingWeapon();
			void ReloadedWeapon();
			
			float GetAimDownState() { return aimDownState; }
			float GetSprintState() { return sprintState; }
		};
		
	}
}