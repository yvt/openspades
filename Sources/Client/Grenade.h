/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

namespace spades {
	namespace client {
		class World;

		class Grenade {
			World *world;
			float fuse;
			Vector3 position;
			Vector3 velocity;

			// FIXME: this actually shouldn't be here because
			//		  the orientation is actually not a part of grenade physics...
			Quaternion orientation;

			void Explode();

			/** @return non-zero if bounced, 2 when sound should be played. */
			int MoveGrenade(float fsynctics);

		public:
			Grenade(World *, Vector3 pos, Vector3 vel, float fuse);
			~Grenade();

			/** @return true when exploded. */
			bool Update(float dt);

			Vector3 GetPosition() { return position; }
			Vector3 GetVelocity() { return velocity; }
			Quaternion GetOrientation() { return orientation; }
			float GetFuse() { return fuse; }
		};
	}
}
