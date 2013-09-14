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
				
				lastSeq[i]->Release(); // renderer owns this
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
