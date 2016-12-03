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

#include "HurtRingView.h"
#include "Client.h"
#include "IImage.h"
#include "IRenderer.h"
#include "Player.h"
#include "World.h"

#include <Core/Debug.h>

namespace spades {
	namespace client {
		HurtRingView::HurtRingView(Client *cli) : client(cli), renderer(cli->GetRenderer()) {
			SPADES_MARK_FUNCTION();

			image = renderer->RegisterImage("Gfx/HurtRing2.png");
		}

		HurtRingView::~HurtRingView() {}

		void HurtRingView::ClearAll() { items.clear(); }

		void HurtRingView::Add(spades::Vector3 dir) {
			SPADES_MARK_FUNCTION();

			Item item;
			item.dir = dir;
			item.fade = 3;
			items.push_back(item);
		}

		void HurtRingView::Update(float dt) {
			SPADES_MARK_FUNCTION();

			std::list<Item>::iterator it;
			std::vector<std::list<Item>::iterator> its;
			for (it = items.begin(); it != items.end(); it++) {
				Item &ent = *it;
				ent.fade -= dt;
				if (ent.fade < 0) {
					its.push_back(it);
				}
			}
			for (size_t i = 0; i < its.size(); i++)
				items.erase(its[i]);
		}

		void HurtRingView::Draw() {
			SPADES_MARK_FUNCTION();

			Vector3 playerFront;
			World *w = client->GetWorld();
			if (!w) {
				ClearAll();
				return;
			}

			Player *p = w->GetLocalPlayer();
			if (p == NULL || !p->IsAlive()) {
				ClearAll();
				return;
			}

			playerFront = p->GetFront2D();

			float hurtRingSize = renderer->ScreenHeight() * .3f;
			float cx = renderer->ScreenWidth() * .5f;
			float cy = renderer->ScreenHeight() * .5f;
			static const float coords[][2] = {{-1, 1}, {1, 1}, {-1, 0}};

			std::list<Item>::iterator it;
			for (it = items.begin(); it != items.end(); it++) {
				Item &item = *it;

				float fade = item.fade * 2.f;
				if (fade > 1.f)
					fade = 1.f;
				Vector4 color = {fade, fade, fade, 0};
				renderer->SetColorAlphaPremultiplied(color);

				Vector3 dir = -item.dir;
				float c = dir.x * playerFront.x + dir.y * playerFront.y;
				float s = dir.y * playerFront.x - dir.x * playerFront.y;

				Vector2 verts[3];
				for (int i = 0; i < 3; i++) {
					verts[i] = MakeVector2(coords[i][0] * c - coords[i][1] * s,
					                       coords[i][0] * s + coords[i][1] * c);
					verts[i] = verts[i] * hurtRingSize + MakeVector2(cx, cy);
				}

				renderer->DrawImage(image, verts[0], verts[1], verts[2],
				                    AABB2(0, 0, image->GetWidth(), image->GetHeight()));
			}
		}
	}
}
