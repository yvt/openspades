//
//  Tracer.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/30/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Tracer.h"
#include "Client.h"
#include "IRenderer.h"

namespace spades {
	namespace client {
		Tracer::Tracer(Client *cli,
					   Vector3 p1,
					   Vector3 p2,
					   float bulletVel) :
		client(cli), startPos(p1), velocity(bulletVel){
			dir = (p2 - p1).Normalize();
			length = (p2 - p1).GetLength();
			
			const float maxTimeSpread = 1.f / 60.f;
			const float shutterTime = .3f / 60.f;
			
			visibleLength = shutterTime * bulletVel;
			curDistance = -visibleLength;
			curDistance += maxTimeSpread * GetRandom();
			
			firstUpdate = true;
			
			image = cli->GetRenderer()->RegisterImage("Gfx/Ball.png");
		}
		
		bool Tracer::Update(float dt) {
			if(!firstUpdate){
				curDistance += dt * velocity;
				if(curDistance > length) {
					return false;
				}
			}
			firstUpdate = false;
			return true;
		}
		
		void Tracer::Render3D() {
			float startDist = curDistance;
			float endDist = curDistance + visibleLength;
			startDist = std::max(startDist, 0.f);
			endDist = std::min(endDist, length);
			if(startDist >= endDist){
				return;
			}
			
			Vector3 pos1 = startPos + dir * startDist;
			Vector3 pos2 = startPos + dir * endDist;
			IRenderer *r = client->GetRenderer();
			Vector4 col = {1.f, .6f, .2f, 0.f};
			r->SetColor(col * 1.3f);
			r->AddLongSprite(image, pos1, pos2, .05f);
		}
		
		Tracer::~Tracer() {
			
		}
	}
}
