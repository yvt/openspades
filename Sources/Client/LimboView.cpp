//
//  LimboView.cpp
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "LimboView.h"
#include "Client.h"
#include "IRenderer.h"
#include "IImage.h"
#include "IFont.h"
#include "World.h"
#include "IAudioDevice.h"

namespace spades{
	namespace client {
		
		
		static float contentsWidth = 800.f;
		
		LimboView::LimboView(Client *client):
		client(client), renderer(client->GetRenderer()){
			// layout now!
			float menuWidth = 200.f;
			float menuHeight = menuWidth / 8.f;
			float rowHeight = menuHeight + 3.f;
			
			float left = (renderer->ScreenWidth() - contentsWidth) * .5f;
			float top = renderer->ScreenHeight() - 150.f;
			
			float teamX = left + 10.f;
			float firstY = top + 35.f;
			
			items.push_back(MenuItem(MenuTeam1,
									 AABB2(teamX,
										   firstY,
										   menuWidth, menuHeight),
									 "Team 1"));
			
			items.push_back(MenuItem(MenuTeam2,
									 AABB2(teamX,
										   firstY + rowHeight,
										   menuWidth, menuHeight),
									 "Team 2")); // TODO: use team name
			items.push_back(MenuItem(MenuTeamSpectator,
									 AABB2(teamX,
										   firstY + rowHeight * 2.f,
										   menuWidth, menuHeight),
									 "Spectator"));
			
			float weapX = left + 260.f;
			
			items.push_back(MenuItem(MenuWeaponRifle,
									 AABB2(weapX,
										   firstY,
										   menuWidth, menuHeight),
									 "Rifle"));
			
			items.push_back(MenuItem(MenuWeaponSMG,
									 AABB2(weapX,
										  firstY + rowHeight,
										   menuWidth, menuHeight),
									 "SMG")); // TODO: use team name
			items.push_back(MenuItem(MenuWeaponShotgun,
									 AABB2(weapX,
										   firstY + rowHeight * 2.f,
										   menuWidth, menuHeight),
									 "Shotgun"));
			
			items.push_back(MenuItem(MenuSpawn,
									 AABB2(left + contentsWidth - 266.f,
										   firstY + 4.f,
										   256.f, 64.f),
									 "Spawn"));
			
			
			
			cursorPos = MakeVector2(renderer->ScreenWidth()*.5f,
									renderer->ScreenHeight()*.5f);
			
			selectedTeam = 2;
			selectedWeapon = RIFLE_WEAPON;
		}
		LimboView::~LimboView() {
			
		}
		
		void LimboView::MouseEvent(float x, float y){
			cursorPos.x += x;
			cursorPos.y += y;
			
			// clip
			float w = renderer->ScreenWidth();
			float h = renderer->ScreenHeight();
			
			cursorPos.x = std::max(cursorPos.x, 0.f);
			cursorPos.y = std::max(cursorPos.y, 0.f);
			cursorPos.x = std::min(cursorPos.x, w);
			cursorPos.y = std::min(cursorPos.y, h);
		}
		
		void LimboView::KeyEvent(const std::string &key){
			if(key == "LeftMouseButton"){
				for(size_t i = 0; i < items.size(); i++){
					MenuItem& item = items[i];
					if(item.hover){
						IAudioDevice *dev = client->audioDevice;
						IAudioChunk *chunk = dev->RegisterSound("Sounds/Feedback/Limbo/Select.wav");
						dev->PlayLocal(chunk, AudioParam());
						switch(item.type){
							case MenuTeam1:
								selectedTeam = 0;
								break;
							case MenuTeam2:
								selectedTeam = 1;
								break;
							case MenuTeamSpectator:
								selectedTeam = 2;
								break;
							case MenuWeaponRifle:
								selectedWeapon = RIFLE_WEAPON;
								break;
							case MenuWeaponSMG:
								selectedWeapon = SMG_WEAPON;
								break;
							case MenuWeaponShotgun:
								selectedWeapon = SHOTGUN_WEAPON;
								break;
							case MenuSpawn:
								client->SpawnPressed();
								break;
						}
					}
				}
				
			}
		}
		
		void LimboView::Update(float dt) {
			// spectator team was actually 255
			if(selectedTeam > 2)
				selectedTeam = 2;
			for(size_t i = 0; i < items.size(); i++){
				MenuItem& item = items[i];
				item.visible = true;
				
				switch(item.type){
					case MenuWeaponRifle:
					case MenuWeaponShotgun:
					case MenuWeaponSMG:
						if(selectedTeam == 2){
							item.visible = false;
						}
					default:;
				}
				
				bool newHover = item.rect && cursorPos;
				if(!item.visible)
					newHover = false;
				if(newHover && !item.hover){
					IAudioDevice *dev = client->audioDevice;
					IAudioChunk *chunk = dev->RegisterSound("Sounds/Feedback/Limbo/Hover.wav");
					dev->PlayLocal(chunk, AudioParam());
				}
				item.hover = newHover;
			}
		}
		
		void LimboView::Draw() {
			IImage *menuItemImage = renderer->RegisterImage("Gfx/Limbo/MenuItem.tga");
			IImage *menuItemBigImage = renderer->RegisterImage("Gfx/Limbo/BigMenuItem.tga");
			//IImage *menuItemRingImage = renderer->RegisterImage("Gfx/Limbo/MenuItemRing.tga");
			IFont *font = client->textFont;
			
			float left = (renderer->ScreenWidth() - contentsWidth) * .5f;
			float top = renderer->ScreenHeight() - 150.f;
			{
				std::string msg = "Select Team:";
				Vector2 pos;
				pos.x = left + 10.f;
				pos.y = top + 10.f;
				font->Draw(msg, pos + MakeVector2(0, 1), 1.f, MakeVector4(0,0,0,0.4));
				font->Draw(msg, pos, 1.f, MakeVector4(1, 1, 1, 1));
			}
			if(selectedTeam != 2){
				std::string msg = "Select Weapon:";
				Vector2 pos;
				pos.x = left + 260.f;
				pos.y = top + 10.f;
				font->Draw(msg, pos + MakeVector2(0, 1), 1.f, MakeVector4(0,0,0,0.4));
				font->Draw(msg, pos, 1.f, MakeVector4(1, 1, 1, 1));
			}
			
			for(size_t i = 0; i < items.size(); i++){
				MenuItem& item = items[i];
				bool selected = false;
				
				if(!item.visible)
					continue;
				
				switch(item.type){
					case MenuTeam1: selected = selectedTeam == 0; break;
					case MenuTeam2: selected = selectedTeam == 1; break;
					case MenuTeamSpectator: selected = selectedTeam == 2; break;
					case MenuWeaponRifle:
						selected = selectedWeapon == RIFLE_WEAPON; break;
					case MenuWeaponSMG:
						selected = selectedWeapon == SMG_WEAPON; break;
					case MenuWeaponShotgun:
						selected = selectedWeapon == SHOTGUN_WEAPON; break;
					default:
						selected = false;
				}
				
				Vector4 fillColor = {0.2, 0.2, 0.2, 0.5};
				Vector4 ringColor = {0, 0, 0, 0};
				
				if(item.hover){
					fillColor = MakeVector4(.4f, .4f, .4f, .7f);
					ringColor = MakeVector4(.8f, .8f, .8f, .7f);
				}
				if(selected){
					fillColor = MakeVector4(.7f, .7f, .7f, .9f);
				}
				
				renderer->SetColor(fillColor);
				if(item.type == MenuSpawn){
					renderer->DrawImage(menuItemBigImage, item.rect);
					
					std::string msg = item.text;
					IFont *bFont = client->textFont;
					Vector2 size = bFont->Measure(msg);
					Vector2 pos;
					pos.x = item.rect.GetMinX() + (item.rect.GetWidth() - size.x) / 2.f + 2.f;
					pos.y = item.rect.GetMinY() + (item.rect.GetHeight() - size.y) / 2.f + 2.f;
					bFont->Draw(msg, pos + MakeVector2(0, 2), 1.f, MakeVector4(0,0,0,0.4));
					bFont->Draw(msg, pos, 1.f, MakeVector4(1, 1, 1, 1));
				}else{
					renderer->DrawImage(menuItemImage, item.rect);
					
					std::string msg = item.text;
					if(item.type == MenuTeam1)
						msg = client->GetWorld()->GetTeam(0).name;
					if(item.type == MenuTeam2)
						msg = client->GetWorld()->GetTeam(1).name;
					Vector2 size = font->Measure(msg);
					Vector2 pos;
					pos.x = item.rect.GetMinX() + 5.f;
					pos.y = item.rect.GetMinY() + (item.rect.GetHeight() - size.y) / 2.f + 2.f;
					font->Draw(msg, pos + MakeVector2(0, 1), 1.f, MakeVector4(0,0,0,0.4));
					font->Draw(msg, pos, 1.f, MakeVector4(1, 1, 1, 1));
				}
			}
			
			IImage *cursor = renderer->RegisterImage("Gfx/Limbo/Cursor.tga");
			
			renderer->SetColor(MakeVector4(1, 1, 1, 1));
			renderer->DrawImage(cursor, AABB2(cursorPos.x - 8,
											  cursorPos.y - 8,
											  32, 32));
		}
	}
}
