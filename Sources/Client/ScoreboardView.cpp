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

#include <algorithm>
#include "ScoreboardView.h"
#include "Client.h"
#include "IRenderer.h"
#include "IImage.h"
#include "World.h"
#include "CTFGameMode.h"
#include "Player.h"
#include "../Core/Debug.h"
#include "IFont.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "TCGameMode.h"
#include "NetClient.h"
#include <Core/Strings.h>
#include "../Core/Settings.h"

SPADES_SETTING(cg_Minimap_Player_Color,"1");

namespace spades {
	namespace client {
		ScoreboardView::ScoreboardView(Client *client):
		client(client), renderer(client->GetRenderer()) {
			SPADES_MARK_FUNCTION();
			ctf = NULL;
			tc = NULL;
		}
		
		ScoreboardView::~ScoreboardView(){
			
		}
		
		int ScoreboardView::GetTeamScore(int team){
			if(ctf){
				return ctf->GetTeam(team).score;
			}else if(tc){
				int cnt = tc->GetNumTerritories();
				int num = 0;
				for(int i = 0; i < cnt; i++)
					if(tc->GetTerritory(i)->ownerTeamId == team)
						num++;
				return num;
			}else{
				return 0;
			}
		}
		
		Vector4 ScoreboardView::GetTeamColor(int team) {
			IntVector3 c = world->GetTeam(team).color;
			return MakeVector4(c.x / 255.f, c.y / 255.f, c.z / 255.f, 1.f);
		}
		
		Vector4 ScoreboardView::AdjustColor(spades::Vector4 col,
											float bright,
											float saturation){
			col.x *= bright;
			col.y *= bright;
			col.z *= bright;
			float avg = (col.x + col.y + col.z) / 3.f;
			col.x = avg + (col.x - avg) * saturation;
			col.y = avg + (col.y - avg) * saturation;
			col.z = avg + (col.z - avg) * saturation;
			
			return col;
		}
		
		void ScoreboardView::Draw() {
			SPADES_MARK_FUNCTION();
			
			world = client->GetWorld();
			if(!world){
				// no world
				return;
			}

			IGameMode* mode = world->GetMode();
			ctf = IGameMode::m_CTF == mode->ModeType() ? static_cast<CTFGameMode *>(mode) : NULL;
			tc = IGameMode::m_TC == mode->ModeType() ? static_cast<TCGameMode *>(mode) : NULL;
			
			Handle<IImage>image;
			IFont *font;
			Vector2 pos, size;
			std::string str;
			float scrWidth = renderer->ScreenWidth();
			//float scrHeight = renderer->ScreenHeight();
			const Vector4 whiteColor = {1,1,1,1};
			Handle<IImage> whiteImage = renderer->RegisterImage("Gfx/White.tga");
			
			float teamBarTop = 120.f;
			float teamBarHeight = 60.f;
			float contentsLeft = scrWidth * .5f - 400.f;
			float contentsRight = scrWidth * .5f + 400.f;
			float playersHeight = 300.f;
			float playersTop = teamBarTop + teamBarHeight;
			float playersBottom = playersTop + playersHeight;
			
			// draw shadow
			image = renderer->RegisterImage("Gfx/Scoreboard/TopShadow.tga");
			size.y = 32.f;
			renderer->SetColorAlphaPremultiplied(MakeVector4(0,0,0,0.2f));
			renderer->DrawImage(image, AABB2(0, teamBarTop-size.y,
											 scrWidth, size.y));
			renderer->SetColorAlphaPremultiplied(MakeVector4(0,0,0,0.2f));
			renderer->DrawImage(image, AABB2(0, playersBottom + size.y,
											 scrWidth, -size.y));
			
			
			// draw team bar
			image = whiteImage;
			renderer->SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(0), 0.8f, 0.3f));
			renderer->DrawImage(image,
								AABB2(0, teamBarTop,
									  scrWidth * .5f, teamBarHeight));
			renderer->SetColorAlphaPremultiplied(AdjustColor(GetTeamColor(1), 0.8f, 0.3f));
			renderer->DrawImage(image,
								AABB2(scrWidth * .5f, teamBarTop,
									  scrWidth * .5f, teamBarHeight));
			
			image = renderer->RegisterImage("Gfx/Scoreboard/Grunt.tga");
			size.x = 120.f; size.y = 60.f;
			renderer->DrawImage(image,
								AABB2(contentsLeft, teamBarTop + teamBarHeight - size.y,
									  size.x, size.y));
			renderer->DrawImage(image,
								AABB2(contentsRight, teamBarTop + teamBarHeight - size.y,
									  -size.x, size.y));
			
			font = client->bigTextFont;
			str = world->GetTeam(0).name;
			pos.x = contentsLeft + 110.f;
			pos.y = teamBarTop + 5.f;
			font->Draw(str, pos + MakeVector2(0, 2), 1.f,
					   MakeVector4(0, 0, 0, 0.5));
			font->Draw(str, pos, 1.f, whiteColor);
			
			str = world->GetTeam(1).name;
			size = font->Measure(str);
			pos.x = contentsRight - 110.f - size.x;
			pos.y = teamBarTop + 5.f;
			font->Draw(str, pos + MakeVector2(0, 2), 1.f,
					   MakeVector4(0, 0, 0, 0.5));
			font->Draw(str, pos, 1.f, whiteColor);
			
			// draw scores
			int capLimit;
			if(ctf) {
				capLimit = ctf->GetCaptureLimit();
			}else if(tc) {
				capLimit = tc->GetNumTerritories();
			}else{
				capLimit = -1;
			}
			if(capLimit != -1) {
				str = Format("{0}-{1}", GetTeamScore(0), capLimit);
				pos.x = scrWidth * .5f - font->Measure(str).x - 15.f;
				pos.y = teamBarTop + 5.f;
				font->Draw(str, pos, 1.f, Vector4(1.f, 1.f, 1.f, 0.5f));
				
				str = Format("{0}-{1}", GetTeamScore(1), capLimit);
				pos.x = scrWidth * .5f + 15.f;
				pos.y = teamBarTop + 5.f;
				font->Draw(str, pos, 1.f, Vector4(1.f, 1.f, 1.f, 0.5f));
			}
			
			// players background
			image = renderer->RegisterImage("Gfx/Scoreboard/PlayersBg.tga");
			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 1.f));
			renderer->DrawImage(image,
								AABB2(0, playersTop,
									  scrWidth, playersHeight));
			
			// draw players
			DrawPlayers(0, contentsLeft, playersTop,
						(contentsRight - contentsLeft) * .5f,
						playersHeight);
			DrawPlayers(1, scrWidth * .5f, playersTop,
						(contentsRight - contentsLeft) * .5f,
						playersHeight);
		}
		
		struct ScoreboardEntry {
			int id;
			int score;
			std::string name;
			bool operator <(const ScoreboardEntry&ent)const{
				return score > ent.score;
			}
		};
		
		void ScoreboardView::DrawPlayers(int team, float left, float top,
										 float width, float height){
			IFont *font = client->textFont;
			float rowHeight = 24.f;
			char buf[256];
			Vector2 size;
			Vector4 white = { 1,1,1,1 };
			int maxRows = (int)floorf(height / rowHeight);
			int numPlayers = 0;
			int cols;
			std::vector<ScoreboardEntry> entries;
			
			for(int i = 0; i < world->GetNumPlayerSlots(); i++){
				Player *p = world->GetPlayer(i);
				if(!p) continue;
				if(p->GetTeamId() != team)
					continue;
					
				ScoreboardEntry ent;
				ent.name = p->GetName();
				ent.score = world->GetPlayerPersistent(i).kills;
				ent.id = i;
				entries.push_back(ent);
				
				numPlayers++;
			}
			
			std::sort(entries.begin(), entries.end());
			
			cols = (numPlayers + maxRows - 1) / maxRows;
			if(cols == 0)cols = 1;
			maxRows = (numPlayers + cols - 1) / cols;
			
			int row = 0, col = 0;
			float colWidth = (float)width / (float)cols;
			
			//definite a palette of 32 color in RGB code
			int palette[32][3]={
						{0,0,0},//0 black
						{255,255,255},//1 white
						{128,128,128},//2 grey
						{255,255,0},//3 yellow
						{0,255,255},//4 cyan-acqua
						{255,0,255},//5 fuchsia						
						{255,0,0},//6 red
						{0,255,0},//7 lime
						{0,0,255},//8 blue
						
						{128,0,0},//9 maroon
						{0,128,0},//10 green
						{0,0,128},//11 navy
						{128,128,0},//12 olive
						{128,0,128},//13 purple
						{0,128,128},//14 teal
						
						{255,128,0},//15 orange
						{255,0,128},//16 deep pink
						{128,0,255},//17 violet
						{0,128,255},//18 bluette
						{128,255,0},//19 lime 2
						{0,255,128},//20 spring green
						
						{255,128,128},//21 light pink
						{128,255,128},//22 light spring
						{128,128,255},//23 light blue			
						{128,255,255},//24 light azure
						{255,255,128},//25 light yellow
						{255,128,255},//26 light pink2
						
						{165,42,42},//27 brown
						{255,69,0},//28 orange	red
						{255,165,0},//29  orange
						
						{139,69,19},//30 maroon medium
						{210,105,30},//31 choccolate
						
			};

			std::string colormode = cg_Minimap_Player_Color;
			
			for(int i = 0; i < numPlayers; i++){
				ScoreboardEntry& ent = entries[i];
				
				float rowY = top + 6.f + row * rowHeight;
				float colX = left + width / (float)cols * (float)col;
				Vector4 color = white;
				if(ent.id == world->GetLocalPlayerIndex())
					color = GetTeamColor(team);
				
				sprintf(buf, "#%d", ent.id); // FIXME: 1-base?
				size = font->Measure(buf);				
				if ( colormode=="1"){
					IntVector3 Colorplayer=IntVector3::Make(palette[i][0],palette[i][1],palette[i][2]);
					Vector4 ColorplayerF = ModifyColor(Colorplayer);
					ColorplayerF *=1.0f;
					font->Draw(buf, MakeVector2(colX + 35.f - size.x,rowY),1.f, ColorplayerF);
				}	
				else {
						font->Draw(buf, MakeVector2(colX + 35.f - size.x,rowY),1.f, color);
				}
				
				font->Draw(ent.name, MakeVector2(colX + 45.f,
											rowY),
						   1.f, color);
				
				sprintf(buf, "%d", ent.score);
				size = font->Measure(buf);
				font->Draw(buf, MakeVector2(colX + colWidth - 10.f - size.x,
											rowY),
						   1.f, color);
				
				row++;
				if(row >= maxRows){
					col++;
					row = 0;
				}
			}
		}
	}
}
