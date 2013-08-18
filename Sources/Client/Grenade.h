//
//  Grenade.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		class World;
		
		class Grenade {
			World *world;
			float fuse;
			Vector3 position;
			Vector3 velocity;
			
			void Explode();
			
			/** @return non-zero if bounced, 2 when sound should be played. */
			int MoveGrenade(float fsynctics);
		public:
			Grenade(World *,
					Vector3 pos,
					Vector3 vel,
					float fuse);
			~Grenade();
			
			/** @return true when exploded. */
			bool Update(float dt);
			
			Vector3 GetPosition() { return position; }
			Vector3 GetVelocity() { return velocity; }
			float GetFuse() { return fuse; }
		};
	}
}
