//
//  SmokeSpriteEntity.cpp
//  OpenSpades
//
//  Created by yvt on 7/21/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "SmokeSpriteEntity.h"
#include "IRenderer.h"
#include "Client.h"
#include "IImage.h"
#include <stdio.h>

namespace spades{
	namespace client{
		static IRenderer *lastRenderer = NULL;
		static IImage *lastSeq[180];
		
		static void Load(IRenderer *r) {
			if(r == lastRenderer)
				return;
			
			for(int i = 0; i < 180; i++){
				char buf[256];
				sprintf(buf, "Textures/Smoke/%03d.tga", i);
				lastSeq[i] = r->RegisterImage(buf);
			}
			
			lastRenderer = r;
		}
		
		static IImage *GetSequence(int i, IRenderer *r){
			Load(r);
			return lastSeq[i];
		}
		
		SmokeSpriteEntity::SmokeSpriteEntity(Client *c,
											 Vector4 color,
											 float fps):
		ParticleSpriteEntity(c, GetSequence(0, c->GetRenderer()), color), fps(fps){
			frame = 0.f;
		}
		
		bool SmokeSpriteEntity::Update(float dt) {
			frame += dt * fps;
			frame = fmodf(frame, 180.f);
			
			int fId = (int)floorf(frame);
			SPAssert(fId >= 0 && fId < 180);
			SetImage(GetSequence(fId, GetRenderer()));
			
			return ParticleSpriteEntity::Update(dt);
		}
	}
}
