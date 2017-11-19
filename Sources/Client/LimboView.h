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

#pragma once

#include <vector>

#include <Core/Math.h>
#include "PhysicsConstants.h"

namespace spades {
	namespace client {
		class Client;
		class IRenderer;

		class LimboView {
			enum MenuType {
				MenuTeam1,
				MenuTeam2,
				MenuTeamSpectator,
				MenuWeaponRifle,
				MenuWeaponSMG,
				MenuWeaponShotgun,
				MenuSpawn
			};
			struct MenuItem {
				MenuType type;
				AABB2 rect;
				std::string text;
				bool hover;
				bool visible;

				MenuItem() {}
				MenuItem(MenuType type, AABB2 rt, std::string txt)
				    : type(type), rect(rt), text(txt), hover(false) {}
			};
			Client *client;
			IRenderer *renderer;

			std::vector<MenuItem> items;

			Vector2 cursorPos;

			int selectedTeam;
			WeaponType selectedWeapon;

		public:
			LimboView(Client *client);
			~LimboView();

			void Update(float dt);
			void MouseEvent(float x, float y);
			void KeyEvent(const std::string &);

			int GetSelectedTeam() { return selectedTeam; }
			WeaponType GetSelectedWeapon() { return selectedWeapon; }
			void SetSelectedTeam(int team) { selectedTeam = team; }
			void SetSelectedWeapon(WeaponType type) { selectedWeapon = type; }

			void Draw();
		};
	}
}
