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

#include <Core/Settings.h>

#include "PaletteView.h"
#include "Client.h"
#include "IImage.h"
#include "IRenderer.h"
#include "NetClient.h"
#include "Player.h"
#include "World.h"

DEFINE_SPADES_SETTING(cg_keyPaletteLeft, "Left");
DEFINE_SPADES_SETTING(cg_keyPaletteRight, "Right");
DEFINE_SPADES_SETTING(cg_keyPaletteUp, "Up");
DEFINE_SPADES_SETTING(cg_keyPaletteDown, "Down");

namespace spades {
	namespace client {
		static IntVector3 SanitizeCol(IntVector3 col) {
			if (col.x < 0)
				col.x = 0;
			if (col.y < 0)
				col.y = 0;
			if (col.z < 0)
				col.z = 0;
			return col;
		}

		PaletteView::PaletteView(Client *client) : client(client), renderer(client->GetRenderer()) {
			IntVector3 cols[] = {{128, 128, 128}, {256, 0, 0},   {256, 128, 0}, {256, 256, 0},
			                     {0, 256, 0},     {0, 256, 256}, {0, 0, 256},   {256, 0, 256}};

			for (int i = 0; i < 8; i++) {
				colors.push_back(SanitizeCol(cols[i] / 8 - 1));
				colors.push_back(SanitizeCol(cols[i] * 3 / 8 - 1));
				colors.push_back(SanitizeCol(cols[i] * 5 / 8 - 1));
				colors.push_back(SanitizeCol(cols[i] * 7 / 8 - 1));

				IntVector3 rem = IntVector3::Make(256, 256, 256);
				rem -= cols[i];

				colors.push_back(cols[i] + rem / 8 - 1);
				colors.push_back(cols[i] + rem * 3 / 8 - 1);
				colors.push_back(cols[i] + rem * 5 / 8 - 1);
				colors.push_back(cols[i] + rem * 7 / 8 - 1);
			}

			defaultColor = 3;
		}

		PaletteView::~PaletteView() {}

		int PaletteView::GetSelectedIndex() {
			World *w = client->GetWorld();
			if (!w)
				return -1;

			Player *p = w->GetLocalPlayer();
			if (!p)
				return -1;

			IntVector3 col = p->GetBlockColor();
			for (int i = 0; i < (int)colors.size(); i++) {
				if (col.x == colors[i].x && col.y == colors[i].y && col.z == colors[i].z)
					return i;
			}
			return -1;
		}

		int PaletteView::GetSelectedOrDefaultIndex() {
			int c = GetSelectedIndex();
			if (c == -1)
				return defaultColor;
			else
				return c;
		}

		void PaletteView::SetSelectedIndex(int idx) {
			IntVector3 col = colors[idx];

			World *w = client->GetWorld();
			if (!w)
				return;

			Player *p = w->GetLocalPlayer();
			if (!p)
				return;

			p->SetHeldBlockColor(col);

			client->net->SendHeldBlockColor();
		}

		bool PaletteView::KeyInput(std::string keyName) {
			if (EqualsIgnoringCase(keyName, cg_keyPaletteLeft)) {
				int c = GetSelectedOrDefaultIndex();
				if (c == 0)
					c = (int)colors.size() - 1;
				else
					c--;
				SetSelectedIndex(c);
				return true;
			} else if (EqualsIgnoringCase(keyName, cg_keyPaletteRight)) {
				int c = GetSelectedOrDefaultIndex();
				if (c == (int)colors.size() - 1)
					c = 0;
				else
					c++;
				SetSelectedIndex(c);
				return true;
			} else if (EqualsIgnoringCase(keyName, cg_keyPaletteUp)) {
				int c = GetSelectedOrDefaultIndex();
				if (c < 8)
					c += (int)colors.size() - 8;
				else
					c -= 8;
				SetSelectedIndex(c);
				return true;
			} else if (EqualsIgnoringCase(keyName, cg_keyPaletteDown)) {
				int c = GetSelectedOrDefaultIndex();
				if (c >= (int)colors.size() - 8)
					c -= (int)colors.size() - 8;
				else
					c += 8;
				SetSelectedIndex(c);
				return true;
			} else {
				return false;
			}
		}

		void PaletteView::Update() {}

		void PaletteView::Draw() {
			Handle<IImage> img = renderer->RegisterImage("Gfx/Palette.png");

			int sel = GetSelectedIndex();

			float scrW = renderer->ScreenWidth();
			float scrH = renderer->ScreenHeight();

			for (size_t phase = 0; phase < 2; phase++) {
				for (size_t i = 0; i < colors.size(); i++) {
					if ((sel == i) != (phase == 1))
						continue;

					int row = static_cast<int>(i / 8);
					int col = static_cast<int>(i % 8);

					bool selected = sel == i;

					// draw color
					IntVector3 icol = colors[i];
					Vector4 cl;
					cl.x = icol.x / 255.f;
					cl.y = icol.y / 255.f;
					cl.z = icol.z / 255.f;
					cl.w = 1.f;

					float x = scrW - 100.f + 10.f * col;
					float y = scrH - 106.f + 10.f * row - 40.f;

					renderer->SetColorAlphaPremultiplied(cl);
					if (selected) {
						renderer->DrawImage(img, MakeVector2(x, y), AABB2(0, 16, 16, 16));
					} else {
						renderer->DrawImage(img, MakeVector2(x, y), AABB2(0, 0, 16, 16));
					}

					renderer->SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1));
					if (selected) {
						renderer->DrawImage(img, MakeVector2(x, y), AABB2(16, 16, 16, 16));
					} else {
						renderer->DrawImage(img, MakeVector2(x, y), AABB2(16, 0, 16, 16));
					}
				}
			}
		}
	}
}
