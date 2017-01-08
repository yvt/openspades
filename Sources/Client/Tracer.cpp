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
		Tracer::Tracer(Client *cli, Vector3 p1, Vector3 p2, float bulletVel)
		    : client(cli), startPos(p1), velocity(bulletVel) {
			dir = (p2 - p1).Normalize();
			length = (p2 - p1).GetLength();

			velocity *= 0.5f; // make it slower for visual effect

			const float maxTimeSpread = 1.0f / 30.f;
			const float shutterTime = 1.0f / 30.f;

			visibleLength = shutterTime * velocity;
			curDistance = -visibleLength;
			curDistance += maxTimeSpread * GetRandom();

			firstUpdate = true;

			image = cli->GetRenderer()->RegisterImage("Gfx/Ball.png");
		}

		bool Tracer::Update(float dt) {
			if (!firstUpdate) {
				curDistance += dt * velocity;
				if (curDistance > length) {
					return false;
				}
			}
			firstUpdate = false;
			return true;
		}

		void Tracer::Render3D() {
			for (float step = 0.0f; step <= 1.0f; step += 0.1f) {
				float startDist = curDistance;
				float endDist = curDistance + visibleLength;

				float midDist = (startDist + endDist) * 0.5f;
				startDist = Mix(startDist, midDist, step);
				endDist = Mix(endDist, midDist, step);

				startDist = std::max(startDist, 0.f);
				endDist = std::min(endDist, length);
				if (startDist >= endDist) {
					continue;
				}

				Vector3 pos1 = startPos + dir * startDist;
				Vector3 pos2 = startPos + dir * endDist;
				IRenderer *r = client->GetRenderer();
				Vector4 col = {1.f, .6f, .2f, 0.f};
				r->SetColorAlphaPremultiplied(col * 0.4f);
				r->AddLongSprite(image, pos1, pos2, .05f);
			}
		}

		Tracer::~Tracer() {}
	}
}
