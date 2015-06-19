/*
 Copyright (c) 2013 OpenSpades Developers
 
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
		class ScriptIViewToolSkin {
			asIScriptObject *obj;
		public:
			ScriptIViewToolSkin(asIScriptObject *obj);
			void SetEyeMatrix(Matrix4 m);
			void SetSwing(Vector3);
			Vector3 GetLeftHandPosition();
			Vector3 GetRightHandPosition();
			void Draw2D();
		};
	}
}