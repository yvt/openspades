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

#include "ScriptFunction.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		
		class ScriptIWeaponSkin {
			asIScriptObject *obj;
		public:
			ScriptIWeaponSkin(asIScriptObject *obj);
			bool ImplementsInterface();
			void SetReadyState(float);
			void SetAimDownSightState(float);
			void SetReloading(bool);
			void SetReloadProgress(float);
			void SetAmmo(int);
			void SetClipSize(int);
			void WeaponFired();
			void ReloadingWeapon();
			void ReloadedWeapon();
		};

		class ScriptIWeaponSkin2 {
			asIScriptObject *obj;
		public:
			ScriptIWeaponSkin2(asIScriptObject *obj);
			bool ImplementsInterface();
			void SetSoundEnvironment(float room, float size, float distance);
			void SetSoundOrigin(Vector3);
		};
	}
}
