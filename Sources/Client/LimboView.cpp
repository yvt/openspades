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

#include <sstream>

#include "Client.h"
#include "Fonts.h"
#include "IAudioChunk.h"
#include "IAudioDevice.h"
#include "IFont.h"
#include "IImage.h"
#include "IRenderer.h"
#include "LimboView.h"
#include "World.h"
#include <Core/Strings.h>

namespace spades {
	namespace client {

		// TODO: make limbo view scriptable using the existing
		//       UI framework.

		static float contentsWidth = 800.f;

		LimboView::LimboView(Client *client) : client(client), renderer(client->GetRenderer()) {
			// layout now!
			float menuWidth = 200.f;
			float menuHeight = menuWidth / 8.f;
			float rowHeight = menuHeight + 3.f;

			float left = (renderer->ScreenWidth() - contentsWidth) * .5f;
			float top = renderer->ScreenHeight() - 150.f;

			float teamX = left + 10.f;
			float firstY = top + 35.f;

			World *w = client->GetWorld();

			items.push_back(MenuItem(MenuTeam1, AABB2(teamX, firstY, menuWidth, menuHeight),
			                         w ? w->GetTeam(0).name : "Team 1"));
			items.push_back(MenuItem(MenuTeam2,
			                         AABB2(teamX, firstY + rowHeight, menuWidth, menuHeight),
			                         w ? w->GetTeam(1).name : "Team 2"));
			items.push_back(MenuItem(MenuTeamSpectator,
			                         AABB2(teamX, firstY + rowHeight * 2.f, menuWidth, menuHeight),
			                         w ? w->GetTeam(2).name : "Spectator"));

			float weapX = left + 260.f;

			items.push_back(MenuItem(MenuWeaponRifle, AABB2(weapX, firstY, menuWidth, menuHeight),
			                         _Tr("Client", "Rifle")));
			items.push_back(MenuItem(MenuWeaponSMG,
			                         AABB2(weapX, firstY + rowHeight, menuWidth, menuHeight),
			                         _Tr("Client", "SMG")));
			items.push_back(MenuItem(MenuWeaponShotgun,
			                         AABB2(weapX, firstY + rowHeight * 2.f, menuWidth, menuHeight),
			                         _Tr("Client", "Shotgun")));

			//! The "Spawn" button that you press when you're ready to "spawn".
			items.push_back(MenuItem(MenuSpawn,
			                         AABB2(left + contentsWidth - 266.f, firstY + 4.f, 256.f, 64.f),
			                         _Tr("Client", "Spawn")));

			cursorPos = MakeVector2(renderer->ScreenWidth() * .5f, renderer->ScreenHeight() * .5f);

			selectedTeam = 2;
			selectedWeapon = RIFLE_WEAPON;
		}
		LimboView::~LimboView() {}

		void LimboView::MouseEvent(float x, float y) {
			cursorPos.x = x;
			cursorPos.y = y;

			// clip
			float w = renderer->ScreenWidth();
			float h = renderer->ScreenHeight();

			cursorPos.x = std::max(cursorPos.x, 0.f);
			cursorPos.y = std::max(cursorPos.y, 0.f);
			cursorPos.x = std::min(cursorPos.x, w);
			cursorPos.y = std::min(cursorPos.y, h);
		}

		void LimboView::KeyEvent(const std::string &key) {
			if (key == "LeftMouseButton") {
				for (size_t i = 0; i < items.size(); i++) {
					MenuItem &item = items[i];
					if (item.hover) {
						IAudioDevice *dev = client->audioDevice;
						Handle<IAudioChunk> chunk =
						  dev->RegisterSound("Sounds/Feedback/Limbo/Select.opus");
						dev->PlayLocal(chunk, AudioParam());
						switch (item.type) {
							case MenuTeam1: selectedTeam = 0; break;
							case MenuTeam2: selectedTeam = 1; break;
							case MenuTeamSpectator: selectedTeam = 2; break;
							case MenuWeaponRifle: selectedWeapon = RIFLE_WEAPON; break;
							case MenuWeaponSMG: selectedWeapon = SMG_WEAPON; break;
							case MenuWeaponShotgun: selectedWeapon = SHOTGUN_WEAPON; break;
							case MenuSpawn: client->SpawnPressed(); break;
						}
					}
				}
			} else if ("1" == key) {
				if (2 == selectedTeam) {
					selectedTeam = 0;
				} else {
					selectedWeapon = RIFLE_WEAPON;
					client->SpawnPressed();
				}
			} else if ("2" == key) {
				if (2 == selectedTeam) {
					selectedTeam = 1;
				} else {
					selectedWeapon = SMG_WEAPON;
					client->SpawnPressed();
				}
			} else if ("3" == key) {
				if (2 != selectedTeam) {
					selectedWeapon = SHOTGUN_WEAPON;
				}
				client->SpawnPressed(); // if we have 3 and are already spec someone wants to spec..
			}
		}

		void LimboView::Update(float dt) {
			// spectator team was actually 255
			if (selectedTeam > 2)
				selectedTeam = 2;
			for (size_t i = 0; i < items.size(); i++) {
				MenuItem &item = items[i];
				item.visible = true;

				switch (item.type) {
					case MenuWeaponRifle:
					case MenuWeaponShotgun:
					case MenuWeaponSMG:
						if (selectedTeam == 2) {
							item.visible = false;
						}
					default:;
				}

				bool newHover = item.rect && cursorPos;
				if (!item.visible)
					newHover = false;
				if (newHover && !item.hover) {
					IAudioDevice *dev = client->audioDevice;
					Handle<IAudioChunk> chunk =
					  dev->RegisterSound("Sounds/Feedback/Limbo/Hover.opus");
					dev->PlayLocal(chunk, AudioParam());
				}
				item.hover = newHover;
			}
		}

		void LimboView::Draw() {
			Handle<IImage> menuItemImage = renderer->RegisterImage("Gfx/Limbo/MenuItem.png");
			Handle<IImage> menuItemBigImage = renderer->RegisterImage("Gfx/Limbo/BigMenuItem.png");
			IFont *font = client->fontManager->GetGuiFont();

			float left = (renderer->ScreenWidth() - contentsWidth) * .5f;
			float top = renderer->ScreenHeight() - 150.f;
			{
				std::string msg = _Tr("Client", "Select Team:");
				Vector2 pos;
				pos.x = left + 10.f;
				pos.y = top + 10.f;
				font->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1),
				                 MakeVector4(0, 0, 0, 0.4f));
			}
			if (selectedTeam != 2) {
				std::string msg = _Tr("Client", "Select Weapon:");
				Vector2 pos;
				pos.x = left + 260.f;
				pos.y = top + 10.f;
				font->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1),
				                 MakeVector4(0, 0, 0, 0.4f));
			}

			for (size_t i = 0; i < items.size(); i++) {
				MenuItem &item = items[i];
				bool selected = false;

				if (!item.visible)
					continue;
				int index = 0;
				switch (item.type) {
					case MenuTeam1:
					case MenuTeam2:
					case MenuTeamSpectator:
						selected = selectedTeam == item.type;
						index = selectedTeam == 2 ? (1 + item.type) : 0;
						break;
					case MenuWeaponRifle:
					case MenuWeaponSMG:
					case MenuWeaponShotgun:
						selected = selectedWeapon == (item.type - 3);
						index = selectedTeam != 2 ? (1 + (item.type - 3)) : 0;
						break;
					default: selected = false;
				}

				Vector4 fillColor = {0.2f, 0.2f, 0.2f, 0.5f};
				Vector4 ringColor = {0, 0, 0, 0};

				if (item.hover) {
					fillColor = MakeVector4(.4f, .4f, .4f, 1.f) * .7f;
					ringColor = MakeVector4(.8f, .8f, .8f, 1.f) * .7f;
				}
				if (selected) {
					fillColor = MakeVector4(.7f, .7f, .7f, 1.f) * .9f;
				}

				renderer->SetColorAlphaPremultiplied(fillColor);
				if (item.type == MenuSpawn) {
					renderer->DrawImage(menuItemBigImage, item.rect);

					std::string msg = item.text;
					IFont *bFont = client->fontManager->GetGuiFont();
					Vector2 size = bFont->Measure(msg);
					Vector2 pos;
					pos.x = item.rect.GetMinX() + (item.rect.GetWidth() - size.x) / 2.f + 2.f;
					pos.y = item.rect.GetMinY() + (item.rect.GetHeight() - size.y) / 2.f;
					bFont->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1),
					                  MakeVector4(0, 0, 0, 0.4f));
				} else {
					renderer->DrawImage(menuItemImage, item.rect);

					std::string msg = item.text;
					if (item.type == MenuTeam1)
						msg = client->GetWorld()->GetTeam(0).name;
					if (item.type == MenuTeam2)
						msg = client->GetWorld()->GetTeam(1).name;
					Vector2 size = font->Measure(msg);
					Vector2 pos;
					pos.x = item.rect.GetMinX() + 5.f;
					pos.y = item.rect.GetMinY() + (item.rect.GetHeight() - size.y) / 2.f;
					font->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1),
					                 MakeVector4(0, 0, 0, 0.4f));
					if (index > 0) {
						std::stringstream ss;
						ss << index;
						msg = ss.str();
						pos.x = item.rect.GetMaxX() - 5.f - font->Measure(msg).x;
						font->DrawShadow(msg, pos, 1.f, MakeVector4(1, 1, 1, 1),
						                 MakeVector4(0, 0, 0, 0.4f));
					}
				}
			}

			Handle<IImage> cursor = renderer->RegisterImage("Gfx/Limbo/Cursor.png");

			renderer->SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1));
			renderer->DrawImage(cursor, AABB2(cursorPos.x - 8, cursorPos.y - 8, 32, 32));
		}
	}
}
