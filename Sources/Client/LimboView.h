//
//  LimboView.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include <vector>
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
			struct MenuItem{
				MenuType type;
				AABB2 rect;
				std::string text;
				bool hover;
				bool visible;
				
				MenuItem(){}
				MenuItem(MenuType type, AABB2 rt, std::string txt):
				type(type), rect(rt), hover(false), text(txt){}
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
			void KeyEvent(const std::string&);
			
			int GetSelectedTeam() { return selectedTeam;}
			WeaponType GetSelectedWeapon() { return selectedWeapon; }
			void SetSelectedTeam(int team) { selectedTeam = team; }
			void SetSelectedWeapon(WeaponType type) { selectedWeapon = type; }
		
			void Draw();
		};
	}
}
