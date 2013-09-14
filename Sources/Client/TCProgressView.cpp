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

#include "TCProgressView.h"
#include "Client.h"
#include "World.h"
#include "TCGameMode.h"
#include "IRenderer.h"
#include "IFont.h"

namespace spades {
	namespace client {
		struct TCProgressState {
			int team1, team2;
			float progress; // 0 = team1 owns
		};
		
		TCProgressView::TCProgressView(Client *c):
		client(c), renderer(c->GetRenderer()){
			lastTerritoryId = -1;
		}
		
		TCProgressView::~TCProgressView() {
			
		}
		
		static TCProgressState StateForTerritory(TCGameMode::Territory *t,
												 int myTeam) {
			TCProgressState state;
			if(t->capturingTeamId == -1){
				state.team1 = t->ownerTeamId;
				state.team2 = 2;
				state.progress = 0.f;
			}else{
				float prg = t->GetProgress();
				state.team1 = t->ownerTeamId;
				state.team2 = t->capturingTeamId;
				state.progress = prg;
				
				if(state.team2 == myTeam) {
					std::swap(state.team1, state.team2);
					state.progress = 1.f - state.progress;
				}
			}
			return state;
		}
		
		void TCProgressView::Draw() {
			World *w = client->GetWorld();
			if(!w){
				lastTerritoryId = -1;
				return;
			}
			TCGameMode *tc = dynamic_cast<TCGameMode *>(w->GetMode());
			if(!tc) return;
			
			float scrW = renderer->ScreenWidth();
			float scrH = renderer->ScreenHeight();
			
			Handle<IImage> prgBg = renderer->RegisterImage("Gfx/TC/ProgressBg.png");
			Handle<IImage> prgBar = renderer->RegisterImage("Gfx/TC/ProgressBar.png");
			
			Player *p = w->GetLocalPlayer();
			if(p && p->GetTeamId() < 2 && p->IsAlive()){
				// show approaching territory
				TCGameMode::Territory *nearTerritory = NULL;
				int nearTerId;
				float distance = 0.f;
				int myTeam = p->GetTeamId();
				
				int cnt = tc->GetNumTerritories();
				for(int i = 0; i < cnt; i++){
					TCGameMode::Territory *t = tc->GetTerritory(i);
					Vector3 diff = t->pos - p->GetEye();
					if(fabsf(diff.x) < 18.f &&
					   fabsf(diff.y) < 18.f &&
					   fabsf(diff.z) < 18.f){
						float dist = diff.GetPoweredLength();
						if(nearTerritory == NULL || dist < distance) {
							nearTerritory = t;
							nearTerId = i;
							distance = dist;
						}
					}
				}
				
				float fade = 1.f;
				if(nearTerritory){
					lastTerritoryId = nearTerId;
					lastTerritoryTime = w->GetTime();
				}else if(lastTerritoryId != -1 &&
						 w->GetTime() < lastTerritoryTime + 2.f){
					fade = 1.f - (w->GetTime() - lastTerritoryTime) / 2.f;
					nearTerritory = tc->GetTerritory(lastTerritoryId);
				}
				
				if(nearTerritory) {
					TCProgressState state = StateForTerritory(nearTerritory, myTeam);
					
					float x = (scrW - 256.f) * .5f;
					float y = scrH * 0.7f;
					
					if(nearTerritory->ownerTeamId == 2){
						renderer->SetColor(MakeVector4(1, 1, 1, fade));
					}else{
						IntVector3 c = w->GetTeam(nearTerritory->ownerTeamId).color;
						renderer->SetColor(MakeVector4(c.x/255.f, c.y/255.f, c.z/255.f, fade));
					}
					renderer->DrawImage(prgBg, MakeVector2(x,y));
					
					// get away from border
					state.progress += (.5f - state.progress) * 12.f / 256.f;
					
					if(state.team1 != 2){
						IntVector3 c = w->GetTeam(state.team1).color;
						renderer->SetColor(MakeVector4(c.x/255.f, c.y/255.f, c.z/255.f, fade*.8f));
						renderer->DrawImage(prgBar, MakeVector2(x, y),
											AABB2(0,0,
														  (1.f-state.progress)*256.f,32));
					}
					
					if(state.team2 != 2){
						IntVector3 c = w->GetTeam(state.team2).color;
						renderer->SetColor(MakeVector4(c.x/255.f, c.y/255.f, c.z/255.f, fade*.8f));
						renderer->DrawImage(prgBar, MakeVector2(x+
																(1.f-state.progress)*256.f, y),
											AABB2((1.f-state.progress)*256.f,0,
												  state.progress*256.f,32));
					}
					
					IFont *font = client->textFont;
					std::string str;
					
					if(nearTerritory->ownerTeamId == 2){
						str = "Neutral Territory";
					}else{
						str = w->GetTeam(nearTerritory->ownerTeamId).name;
						str += "'s Territory";
					}
					
					Vector2 size = font->Measure(str);
					x = (scrW - size.x) * .5f;
					y += 35.f;
					
					font->Draw(str, MakeVector2(x+1, y+1), 1.f,
							   MakeVector4(0, 0, 0, 0.5f*fade));
					font->Draw(str, MakeVector2(x, y), 1.f,
							   MakeVector4(1.f, 1.f, 1.f, fade));
				}
			}else{
				// unable to show nearby territory
				lastTerritoryId = -1;
			}
		}
	}
}
