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

#include "ChatWindow.h"
#include "IRenderer.h"
#include "IFont.h"
#include <Core/Debug.h>
#include "Client.h"
#include "World.h"
#include <Core/Exception.h>
#include <ctype.h>


namespace spades {
	namespace client{
		
		ChatWindow::ChatWindow(Client *cli, IRenderer* rend, IFont *fnt, bool killfeed):
		client(cli), renderer(rend),
		font(fnt), killfeed(killfeed){
			firstY = 0.f;
			mKillImages.push_back( renderer->RegisterImage("Killfeed/a-Rifle.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/b-SMG.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/c-Shotgun.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/d-Headshot.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/e-Melee.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/f-Grenade.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/g-Falling.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/h-Teamchange.png") );
			mKillImages.push_back( renderer->RegisterImage("Killfeed/i-Classchange.png") );
			for( size_t n = 0; n < mKillImages.size(); ++n ) {
				if( mKillImages[n]->GetHeight() > GetLineHeight() ) {
					SPRaise( "Kill image (%d) height too big ", n );
				}
			}
		}
		ChatWindow::~ChatWindow(){}
		
		float ChatWindow::GetWidth() {
			return renderer->ScreenWidth() / 2;
		}
		
		float ChatWindow::GetHeight() {
			return renderer->ScreenHeight() / 3;
		}
		
		float ChatWindow::GetLineHeight() {
			return 20.f;
		}
		
		static bool isWordChar(char c){
			return isalnum(c) || c == '\'';
		}

		std::string ChatWindow::killImage( int type, int weapon )
		{
			std::string tmp = "xx";
			tmp[0] = 7;
			switch( type ) {
				case KillTypeWeapon:
					switch( weapon ) {
						case 0: case 1: case 2:
							tmp[1] = 'a' + weapon;
							break;
						default:
							return "";
					}
					break;
				case KillTypeHeadshot: case KillTypeMelee: case KillTypeGrenade:
				case KillTypeFall: case KillTypeTeamChange: case KillTypeClassChange:
					tmp[1] = 'a' + 2 + type;
					break;
				default:
					return "";
			}
			return tmp;
		}

		void ChatWindow::AddMessage(const std::string &msg){
			SPADES_MARK_FUNCTION();
			
			// get visible message string
			std::string str;
			float x = 0.f, maxW = GetWidth();
			float lh = GetLineHeight(), h = lh;
			size_t wordStart = std::string::npos;
			size_t wordStartOutPos;
			
			for(size_t i = 0; i < msg.size(); i++){
				if(msg[i] > MsgColorMax &&
				   msg[i] != 13 && msg[i] != 10){
					if(isWordChar(msg[i])){
						if(wordStart == std::string::npos){
							wordStart = msg.size();
							wordStartOutPos = str.size();
						}
					}else{
						wordStart = std::string::npos;
					}
					
					float w = font->Measure(std::string(&msg[i],1)).x;
					if(x + w > maxW){
						if(wordStart != std::string::npos && wordStart != str.size()){
							// adding a part of word.
							// do word wrapping
							std::string s = msg.substr(wordStart, i - wordStart + 1);
							float nw = font->Measure(s).x;
							if(nw <= maxW){
								// word wrap succeeds
								w = nw;
								x = w;
								h += lh;
								str.insert(wordStartOutPos, "\n");
								
								goto didWordWrap;
							}
							
						}
						x = 0; h += lh;
						str += 13;
					}
					x += w;
					str += msg[i];
				didWordWrap:;
				}else if(msg[i] == 13 || msg[i] == 10){
					x = 0; h += lh;
					str += 13;
				}else{
					str += msg[i];
				}
			}
			
			entries.push_front( ChatEntry( msg, h, 0.f, 15.f ) );
			
			firstY -= h;
		}
		
		std::string ChatWindow::ColoredMessage(const std::string & msg, char c){
			SPADES_MARK_FUNCTION_DEBUG();
			std::string s;
			s += c;
			s += msg;
			s += MsgColorRestore;
			return s;
		}
		
		std::string ChatWindow::TeamColorMessage(const std::string &msg, int team){
			SPADES_MARK_FUNCTION_DEBUG();
			switch( team ) {
			case 0:
				return ColoredMessage(msg, MsgColorTeam1);
			case 1:
				return ColoredMessage(msg, MsgColorTeam2);
			case 2:
				return ColoredMessage(msg, MsgColorTeam3);
			default:
				return msg;
			}
		}
		
		static Vector4 ConvertColor(IntVector3 v){
			return MakeVector4((float)v.x / 255.f, (float)v.y / 255.f, (float)v.z / 255.f, 1.f);
		}
		
		Vector4 ChatWindow::GetColor(char c){
			World* w = client ? client->GetWorld() : NULL;
			switch(c){
				case MsgColorTeam1:
					return w ? ConvertColor(w->GetTeam(0).color) : MakeVector4( 0, 1, 0, 1 ); 
				case MsgColorTeam2:
					return w ? ConvertColor(w->GetTeam(1).color) : MakeVector4( 0, 0, 1, 1 );
				case MsgColorTeam3:
					return w ? ConvertColor(w->GetTeam(2).color) : MakeVector4( 1, 1, 0, 1 );
				case MsgColorRed:
					return MakeVector4(1,0,0,1);
				case MsgColorSysInfo:
					return MakeVector4(0,1,0,1);
				default:
					return MakeVector4(1,1,1,1);
			}
		}
		
		void ChatWindow::Update(float dt) {
			
			if(firstY < 0.f){
				firstY += dt * std::max(100.f, -firstY);
				if(firstY > 0.f)
					firstY = 0.f;
			}

			float height = GetHeight();
			float y = firstY;

			for(std::list<ChatEntry>::iterator it = entries.begin(); it != entries.end(); ){
				ChatEntry& ent = *it;
				if(y + ent.height > height){
					// should fade out
					ent.fade -= dt * 4.f;
					if(ent.fade < 0.f){
						ent.fade = 0.f;
						std::list<ChatEntry>::iterator er = it++;
						entries.erase( er );
						continue;
					}
				}else if(y + ent.height > 0.f){
					// should fade in
					ent.fade += dt * 4.f;
					if(ent.fade > 1.f)
						ent.fade = 1.f;
				}
				
				ent.timeFade -= dt;
				if(ent.timeFade < 0.f){
					std::list<ChatEntry>::iterator er = it++;
					entries.erase( er );
					continue;
				}
				
				y += ent.height;
				++it;
			}
		}
		
		IImage* ChatWindow::imageForIndex( char index )
		{
			int real = index - 'a';
			if( real >= 0 && real < mKillImages.size() ) {
				return mKillImages[real];
			}
			return NULL;
		}

		void ChatWindow::Draw() {
			SPADES_MARK_FUNCTION();
			
			float winX = 4.f;
			float winY = killfeed ? 8.f :
			(float)renderer->ScreenHeight() * .5f;
			std::list<ChatEntry>::iterator it;
			
			float lHeight = GetLineHeight();
			
			float y = firstY;
			
			Vector4 shadowColor = {0, 0, 0, 0.5};
			
			for(it = entries.begin(); it != entries.end(); ++it){
				ChatEntry& ent = *it;
				
				std::string msg = ent.msg;
				Vector4 color = GetColor(MsgColorRestore);
				float tx = 0.f, ty = y;
				float fade = ent.fade;
				if(ent.timeFade < 1.f) { fade *= ent.timeFade; }
				shadowColor.w = .5f * fade;
				color.w *= fade;
				std::string ch = "a";	//let's not make a new object for each character.
				for(size_t i = 0; i < msg.size(); i++){
					if(msg[i] == 13 || msg[i] == 10){
						tx = 0.f; ty += lHeight;
					}else if(msg[i] <= MsgColorMax){
						if( msg[i] == MsgImage ) {
							IImage* kill = NULL;
							if( i+1 < msg.size() && (kill = imageForIndex(msg[i+1]))  ) {
								renderer->DrawImage( kill, MakeVector2( tx + winX, ty + winY ) );
								tx += kill->GetWidth();
								++i;
							} else {
								// just ignore invalid icon specifier
							}
						} else {
							color = GetColor(msg[i]);
							color.w *= fade;
						}
					}else{
						ch[0] = msg[i];
						font->DrawShadow(ch, MakeVector2(tx + winX, ty + winY), 1.f, color, shadowColor);
						tx += font->Measure(ch).x;
					}
				}
				
				y += ent.height;
			}
		}
	}
}

