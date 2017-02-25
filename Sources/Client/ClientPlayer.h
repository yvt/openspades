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

#include "Player.h"
#include <Core/Math.h>
#include <Core/RefCountedObject.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {

	class ScriptFunction;

	namespace client {

		class Client;
		class IRenderer;
		class IAudioDevice;

		class SandboxedRenderer;

		/** Representation of player which is used by
		 * drawing/view layer of game client. */
		class ClientPlayer : public RefCountedObject {
			Client *client;
			Player *player;

			float sprintState;
			float aimDownState;
			float toolRaiseState;
			Player::ToolType currentTool;
			float localFireVibrationTime;
			float time;

			/**
			 * Indicates whether the third-person weapon skin script has the latest
			 * value of `OriginMatrix`.
			 */
			bool hasValidOriginMatrix;

			Vector3 viewWeaponOffset;

			Vector3 lastFront;

			asIScriptObject *spadeSkin;
			asIScriptObject *blockSkin;
			asIScriptObject *weaponSkin;
			asIScriptObject *grenadeSkin;

			asIScriptObject *spadeViewSkin;
			asIScriptObject *blockViewSkin;
			asIScriptObject *weaponViewSkin;
			asIScriptObject *grenadeViewSkin;

			Handle<SandboxedRenderer> sandboxedRenderer;

			void AddToSceneThirdPersonView();
			void AddToSceneFirstPersonView();

			void SetSkinParameterForTool(Player::ToolType, asIScriptObject *);
			void SetCommonSkinParameter(asIScriptObject *);

			struct AmbienceInfo;
			AmbienceInfo ComputeAmbience();

			float GetLocalFireVibration();

			bool ShouldRenderInThirdPersonView();

			asIScriptObject *initScriptFactory(ScriptFunction &creator, IRenderer *renderer,
			                                   IAudioDevice *audio);

		protected:
			~ClientPlayer();

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

			Matrix4 GetEyeMatrix();
		};
	}
}
