//
//  Grenade.cpp
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Grenade.h"
#include "GameMap.h"
#include "World.h"
#include "PhysicsConstants.h"
#include "IWorldListener.h"
#include "../Core/Debug.h"

namespace spades {
	namespace client {
		Grenade::Grenade(World *w,
						 Vector3 pos,
						 Vector3 vel,
						 float fuse){
			SPADES_MARK_FUNCTION();
			
			position = pos;
			velocity = vel;
			this->fuse = fuse;
			world = w;
		}
		
		Grenade::~Grenade(){
			SPADES_MARK_FUNCTION();
		}
		
		bool Grenade::Update(float dt) {
			SPADES_MARK_FUNCTION();
			
			fuse -= dt;
			if(fuse < 0.f){
				Explode();
				return true;
			}
			
			if(MoveGrenade(dt) == 2){
				if(world->GetListener())
					world->GetListener()->GrenadeBounced(this);
			}
			
			return false;
		}
		
		void Grenade::Explode() {
			SPADES_MARK_FUNCTION();
			
			if(world->GetListener())
				world->GetListener()->GrenadeExploded(this);
		}
		
		int Grenade::MoveGrenade(float fsynctics) {
			SPADES_MARK_FUNCTION();
			
			float f = fsynctics * 32.f;
			Vector3 oldPos = position;
			velocity.z += fsynctics;
			position += velocity * f;
			
			IntVector3 lp = position.Floor();
			IntVector3 lp2 = oldPos.Floor();
			GameMap *m = world->GetMap();
			
			if(lp.z >= 63 && lp2.z < 63) {
				if(world->GetListener())
					world->GetListener()->GrenadeDroppedIntoWater(this);
			}
			
			int ret = 0;
			if(m->ClipWorld(position.x, position.y, position.z)){
				// hit a wall
				ret = 1;
				if(fabsf(velocity.x) > BOUNCE_SOUND_THRESHOLD ||
				   fabsf(velocity.y) > BOUNCE_SOUND_THRESHOLD ||
				   fabsf(velocity.z) > BOUNCE_SOUND_THRESHOLD)
					ret = 2;
				
				if (lp.z != lp2.z && ((lp.x == lp2.x && lp.y == lp2.y) ||
									  !m->ClipWorld(lp.x, lp.y, lp2.z)))
					velocity.z = -velocity.z;
				else if(lp.x != lp2.x && ((lp.y == lp2.y && lp.z == lp2.z) ||
										  !m->ClipWorld(lp2.x, lp.y, lp.z)))
					velocity.x = -velocity.x;
				else if(lp.y != lp2.y && ((lp.x == lp2.x && lp.z == lp2.z) ||
										  !m->ClipWorld(lp.x, lp2.y, lp.z)))
					velocity.y = -velocity.y;
				position = oldPos;
				velocity *= .36f;
			}
			return ret;
		}
	}
}
