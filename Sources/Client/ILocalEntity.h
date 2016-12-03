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

namespace spades {
	namespace client {
		class ILocalEntity {
		public:
			virtual ~ILocalEntity() {}
			/** @return false if this entity should be removed from the scene. */
			virtual bool Update(float dt) = 0;
			virtual void Render3D() {}
			virtual void Render2D() {}
		};
	}
}
