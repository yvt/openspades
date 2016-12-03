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

#include <cstdio>

#include "Client.h"
#include "IImage.h"
#include "IRenderer.h"
#include "SmokeSpriteEntity.h"

namespace spades {
	namespace client {
		static IRenderer *lastRenderer = NULL;
		static IImage *lastSeq[180];
		static IImage *lastSeq2[48];

		// FIXME: add "image manager"?
		static void Load(IRenderer *r) {
			if (r == lastRenderer)
				return;

			for (int i = 0; i < 180; i++) {
				char buf[256];
				sprintf(buf, "Textures/Smoke1/%03d.png", i);
				lastSeq[i] = r->RegisterImage(buf);
			}
			for (int i = 0; i < 48; i++) {
				char buf[256];
				sprintf(buf, "Textures/Smoke2/%03d.png", i);
				lastSeq2[i] = r->RegisterImage(buf);
			}

			lastRenderer = r;
		}

		IImage *SmokeSpriteEntity::GetSequence(int i, IRenderer *r, Type type) {
			Load(r);
			if (type == Type::Steady) {
				SPAssert(i >= 0 && i < 180);
				return lastSeq[i];
			} else {
				SPAssert(i >= 0 && i < 48);
				return lastSeq2[i];
			}
		}

		SmokeSpriteEntity::SmokeSpriteEntity(Client *c, Vector4 color, float fps, Type type)
		    : ParticleSpriteEntity(c, GetSequence(0, c->GetRenderer(), type), color),
		      fps(fps),
		      type(type) {
			frame = 0.f;
		}

		void SmokeSpriteEntity::Preload(IRenderer *r) { Load(r); }

		bool SmokeSpriteEntity::Update(float dt) {
			frame += dt * fps;
			if (type == Type::Steady) {
				frame = fmodf(frame, 180.f);
			} else {
				if (frame > 47.f) {
					frame = 47.f;
					return false;
				}
			}

			int fId = (int)floorf(frame);
			SetImage(GetSequence(fId, GetRenderer(), type));

			return ParticleSpriteEntity::Update(dt);
		}
	}
}
