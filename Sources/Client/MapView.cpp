//
//  MapView.cpp
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "MapView.h"
#include "IRenderer.h"
#include "IImage.h"
#include "Player.h"
#include "World.h"
#include "CTFGameMode.h"
#include "Client.h"
#include "GameMap.h"
#include "TCGameMode.h"

namespace spades {
	namespace client {
		MapView::MapView(Client *c):
		client(c), renderer(c->GetRenderer()){
			scaleMode = 2;
			actualScale = 1.f;
			lastScale = 1.f;
		}
		
		MapView::~MapView(){
			
		}
		
		void MapView::Update(float dt){
			float scale;
			switch(scaleMode){
				case 0: // 400%
					scale = 1.f / 4.f;
					break;
				case 1: // 200%
					scale = 1.f / 2.f;
					break;
				case 2: // 100%
					scale = 1.f;
					break;
				case 3: // 50%
					scale = 2.f;
					break;
				default:
					SPAssert(false);
			}
			if(actualScale != scale){
				float spd = fabsf(scale - lastScale) * 4.f;
				spd = std::max(spd, 0.2f);
				spd *= dt;
				if(scale > actualScale){
					actualScale += spd;
					if(actualScale > scale)
						actualScale = scale;
				}else{
					actualScale -= spd;
					if(actualScale < scale)
						actualScale = scale;
				}
			}
		}
		
		void MapView::DrawIcon(spades::Vector3 pos,
							   spades::client::IImage *img,
							   float rotation){
			if(pos.x < inRect.GetMinX() ||
			   pos.x > inRect.GetMaxX() ||
			   pos.y < inRect.GetMinY() ||
			   pos.y > inRect.GetMaxY())
				return;
			
			Vector2 scrPos;
			scrPos.x = (pos.x - inRect.GetMinX()) / inRect.GetWidth();
			scrPos.x = (scrPos.x * outRect.GetWidth()) + outRect.GetMinX();
			scrPos.y = (pos.y - inRect.GetMinY()) / inRect.GetHeight();
			scrPos.y = (scrPos.y * outRect.GetHeight()) + outRect.GetMinY();
			
			float c = rotation != 0.f ? cosf(rotation) : 1.f;
			float s = rotation != 0.f ? sinf(rotation) : 0.f;
			static const float coords[][2] = {{-1, -1}, {1, -1}, {-1, 1}};
			Vector2 u = MakeVector2(img->GetWidth() * .5f, 0.f);
			Vector2 v = MakeVector2(0.f, img->GetHeight() * .5f);
			
			Vector2 vt[3];
			for(int i = 0; i < 3; i++){
				Vector2 ss = u * coords[i][0] + v * coords[i][1];
				vt[i].x = scrPos.x + ss.x * c - ss.y * s;
				vt[i].y = scrPos.y + ss.x * s + ss.y * c;
			}
			
			renderer->DrawImage(img, vt[0], vt[1], vt[2],
								AABB2(0, 0, img->GetWidth(), img->GetHeight()));
		}
		
		void MapView::SwitchScale() {
			scaleMode = (scaleMode + 1) % 4;
			
			lastScale = actualScale;
		}
		
		void MapView::Draw(){
			World *world = client->GetWorld();
			if(!world)
				return;
			
			Player *player = world->GetLocalPlayer();
			if(client->IsFollowing()){
				player = world->GetPlayer(client->followingPlayerId);
			}
			if(!player)
				return;
			
			if(!player->IsAlive())
				return;
			
			
			
			GameMap *map = world->GetMap();
			
			Vector3 pos = player->GetPosition();;
			if(player->GetTeamId() >= 2){
				pos = client->followPos;
			}
			Vector2 center = {pos.x, pos.y};
			Vector2 mapWndSize = {256, 256};
			Vector2 inRange = mapWndSize * .5f * actualScale;
			Vector2 mapSize = MakeVector2(map->Width(), map->Height());
			AABB2 inRect(center - inRange, center + inRange);
			if(inRect.GetMinX() < 0.f)
				inRect = inRect.Translated(-inRect.GetMinX(), 0.f);
			if(inRect.GetMinY() < 0.f)
				inRect = inRect.Translated(0, -inRect.GetMinY());
			if(inRect.GetMaxX() > mapSize.x)
				inRect = inRect.Translated(mapSize.x - inRect.GetMaxX(), 0.f);
			if(inRect.GetMaxY() > mapSize.y)
				inRect = inRect.Translated(0, mapSize.y - inRect.GetMaxY());
			
			AABB2 outRect(renderer->ScreenWidth() - mapWndSize.x - 16.f, 16.f,
						  mapWndSize.x,
						  mapWndSize.y);
			renderer->SetColor(MakeVector4(1,1,1,1));
			renderer->DrawFlatGameMap(outRect, inRect);
			
			this->inRect = inRect;
			this->outRect = outRect;
			
			// draw grid
			
			renderer->SetColor(MakeVector4(0,0,0,0.8));
			IImage *dashLine = renderer->RegisterImage("Gfx/DashLine.tga");
			for(float x = 64.f; x < map->Width(); x += 64.f){
				float wx = (x - inRect.GetMinX()) / inRect.GetWidth();
				if(wx < 0.f || wx > 1.f)
					continue;
				wx = (wx * outRect.GetWidth()) + outRect.GetMinX();
				wx = roundf(wx);
				renderer->DrawImage(dashLine,
									MakeVector2(wx, outRect.GetMinY()),
									AABB2(0, 0, 1.f, outRect.GetHeight()));
			}
			for(float y = 64.f; y < map->Height(); y += 64.f){
				float wy = (y - inRect.GetMinY()) / inRect.GetHeight();
				if(wy < 0.f || wy > 1.f)
					continue;
				wy = (wy * outRect.GetHeight()) + outRect.GetMinY();
				wy = roundf(wy);
				renderer->DrawImage(dashLine,
									MakeVector2(outRect.GetMinX(), wy),
									AABB2(0, 0, outRect.GetWidth(), 1.f));
			}
			
			// draw grid label
			renderer->SetColor(MakeVector4(1,1,1,0.8));
			IImage *mapFont = renderer->RegisterImage("Gfx/Fonts/MapFont.tga");
			for(int i = 0; i < 8; i++){
				float startX = (float)i * 64.f;
				float endX = startX + 64.f;
				if(startX > inRect.GetMaxX() ||
				   endX < inRect.GetMinX())
					continue;
				float fade = std::min((std::min(endX, inRect.GetMaxX()) -
									   std::max(startX, inRect.GetMinX())) /
									  (endX - startX) * 2.f, 1.f);
				renderer->SetColor(MakeVector4(1,1,1,fade * .8f));
				
				float center = std::max(startX, inRect.GetMinX());
				center = .5f * (center + std::min(endX, inRect.GetMaxX()));
				
				float wx = (center - inRect.GetMinX()) / inRect.GetWidth();
				wx = (wx * outRect.GetWidth()) + outRect.GetMinX();
				wx = roundf(wx);
				
				int fntX = (i & 3) * 8;
				int fntY = (i >> 2) * 8;
				renderer->DrawImage(mapFont,
									MakeVector2(wx - 4.f, outRect.GetMinY() + 4),
									AABB2(fntX, fntY, 8, 8));
			}
			for(int i = 0; i < 8; i++){
				float startY = (float)i * 64.f;
				float endY = startY + 64.f;
				if(startY > inRect.GetMaxY() ||
				   endY < inRect.GetMinY())
					continue;
				float fade = std::min((std::min(endY, inRect.GetMaxY()) -
									   std::max(startY, inRect.GetMinY())) /
									  (endY - startY) * 2.f, 1.f);
				renderer->SetColor(MakeVector4(1,1,1,fade * .8f));
				
				float center = std::max(startY, inRect.GetMinY());
				center = .5f * (center + std::min(endY, inRect.GetMaxY()));
				
				float wy = (center - inRect.GetMinY()) / inRect.GetHeight();
				wy = (wy * outRect.GetHeight()) + outRect.GetMinY();
				wy = roundf(wy);
				
				int fntX = (i & 3) * 8;
				int fntY = (i >> 2) * 8 + 16;
				renderer->DrawImage(mapFont,
									MakeVector2(outRect.GetMinX() + 4, wy - 4.f),
									AABB2(fntX, fntY, 8, 8));
			}
			
			//draw objects
			IImage *playerIcon = renderer->RegisterImage("Gfx/Player.tga");
			
			{
				
				IntVector3 teamColor = world->GetTeam(world->GetLocalPlayer()->GetTeamId()).color;
				Vector4 teamColorF = {teamColor.x /255.f,
					teamColor.y / 255.f, teamColor.z / 255.f, 1.f};
				for(int i = 0; i < world->GetNumPlayerSlots(); i++){
					Player * p = world->GetPlayer(i);
					if(p == NULL ||
					   p->GetTeamId() != world->GetLocalPlayer()->GetTeamId() ||
					   !p->IsAlive())
						continue;
					
					Vector3 front = p->GetFront2D();
					float ang = atan2(front.x, -front.y);
					if(player->GetTeamId() >= 2){
						ang = client->followYaw - M_PI*.5f;
					}
					
					if(p == world->GetLocalPlayer())
						renderer->SetColor(MakeVector4(0,1,1,1));
					else{
						renderer->SetColor(teamColorF);
					}
					DrawIcon(player->GetTeamId() >= 2 ?
							 client->followPos :
							 p->GetPosition(), playerIcon, ang);
				}
			}
			
			CTFGameMode *ctf = dynamic_cast<CTFGameMode *>(world->GetMode());
			if(ctf){
				IImage *intelIcon = renderer->RegisterImage("Gfx/Intel.tga");
				IImage *baseIcon = renderer->RegisterImage("Gfx/CTFBase.tga");
				IImage *medicalIcon = renderer->RegisterImage("Gfx/Medical.tga");
				for(int tId = 0; tId < 2; tId++){
					CTFGameMode::Team& team = ctf->GetTeam(tId);
					IntVector3 teamColor = world->GetTeam(tId).color;
					Vector4 teamColorF = {teamColor.x /255.f,
						teamColor.y / 255.f, teamColor.z / 255.f, 1.f};
					
					// draw base
					renderer->SetColor(teamColorF);
					DrawIcon(team.basePos, baseIcon, 0.f);
					
					renderer->SetColor(MakeVector4(1,1,1,1));
					DrawIcon(team.basePos, medicalIcon, 0.f);
					
					
					// draw flag
					if(!ctf->GetTeam(1-tId).hasIntel){
						renderer->SetColor(teamColorF);
						DrawIcon(team.flagPos, intelIcon, 0.f);
					}else if(world->GetLocalPlayer()->GetTeamId() == 1-tId){
						// local player's team is carrying
						int cId = ctf->GetTeam(1-tId).carrier;
						
						// in some game modes, carrier becomes invalid
						if(cId < world->GetNumPlayerSlots()){
							Player * carrier= world->GetPlayer(cId);
							if(carrier && carrier->GetTeamId() ==
							   world->GetLocalPlayer()->GetTeamId()){
								
								Vector4 col = teamColorF;
								col.w = fabsf(sinf(world->GetTime() * 4.f));
								renderer->SetColor(col);
								DrawIcon(carrier->GetPosition(), intelIcon, 0.f);
							}
						}
					}
				}
			}
		
			TCGameMode *tc = dynamic_cast<TCGameMode *>(world->GetMode());
			if(tc){
				IImage *icon = renderer->RegisterImage("Gfx/TCTerritory.tga");
				int cnt = tc->GetNumTerritories();
				for(int i = 0; i < cnt; i++){
					TCGameMode::Territory *t = tc->GetTerritory(i);
					IntVector3 teamColor = {0,0,0};
					if(t->ownerTeamId < 2){
						teamColor = world->GetTeam(t->ownerTeamId).color;
					}
					Vector4 teamColorF = {teamColor.x /255.f,
						teamColor.y / 255.f, teamColor.z / 255.f, 1.f};
					
					// draw base
					renderer->SetColor(teamColorF);
					DrawIcon(t->pos, icon, 0.f);
					
				}
			}
		}
	}
}
