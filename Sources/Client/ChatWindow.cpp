//
//  ChatWindow.cpp
//  OpenSpades
//
//  Created by yvt on 7/18/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "ChatWindow.h"
#include "IRenderer.h"
#include "IFont.h"
#include "../Core/Debug.h"
#include "Client.h"
#include "World.h"
#include "../Core/Exception.h"
#include <ctype.h>

namespace spades {
	namespace client{
		
		ChatWindow::ChatWindow(Client *cli, IFont *fnt):
		client(cli), renderer(cli->GetRenderer()),
		font(fnt){
			firstY = 0.f;
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
		
		void ChatWindow::AddMessage(const std::string &msg){
			SPADES_MARK_FUNCTION();
			
			ChatEntry ent;
			
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
						if(wordStart != std::string::npos &&
						   wordStart != str.size()){
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
			
			ent.height = h;
			ent.msg = msg;
			ent.fade = 0.f;
			ent.timeFade = 15.f;
			
			entries.push_front(ent);
			
			firstY -= h;
		}
		
		std::string ChatWindow::ColoredMessage(const std::string & msg,
											   char c){
			SPADES_MARK_FUNCTION_DEBUG();
			std::string s;
			s += c;
			s += msg;
			s += MsgColorRestore;
			return s;
		}
		
		std::string ChatWindow::TeamColorMessage(const std::string &msg,
												 int team){
			SPADES_MARK_FUNCTION_DEBUG();
			if(team == 0)
				return ColoredMessage(msg, MsgColorTeam1);
			if(team == 1)
				return ColoredMessage(msg, MsgColorTeam2);
			if(team == 2)
				return ColoredMessage(msg, MsgColorTeam3);
			return msg;
		}
		
		static Vector4 ConvertColor(IntVector3 v){
			return MakeVector4((float)v.x / 255.f,
						   (float)v.y / 255.f,
						   (float)v.z / 255.f,
						   1.f);
		}
		
		Vector4 ChatWindow::GetColor(char c){
			switch(c){
				case MsgColorTeam1:
					if(client->GetWorld())
						return ConvertColor(client->GetWorld()->GetTeam(0).color);
				case MsgColorTeam2:
					if(client->GetWorld())
						return ConvertColor(client->GetWorld()->GetTeam(1).color);
				case MsgColorTeam3:
					if(client->GetWorld())
						return ConvertColor(client->GetWorld()->GetTeam(2).color);
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
			
			std::list<ChatEntry>::iterator it;
			float height = GetHeight();
			float y = firstY;
			
			std::vector<std::list<ChatEntry>::iterator> removedItems;
			
			for(it = entries.begin(); it != entries.end(); it++){
				ChatEntry& ent = *it;
				if(y + ent.height > height){
					// should fade out
					ent.fade -= dt * 4.f;
					if(ent.fade < 0.f){
						ent.fade = 0.f;
						removedItems.push_back(it);
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
					removedItems.push_back(it);
					continue;
				}
				
				y += ent.height;
			}
			
			for(size_t i = 0; i < removedItems.size(); i++)
				entries.erase(removedItems[i]);
		}
		
		void ChatWindow::Draw() {
			SPADES_MARK_FUNCTION();
			
			float winX = 4.f;
			float winY = (float)renderer->ScreenHeight() * .5f;
			std::list<ChatEntry>::iterator it;
			
			float lHeight = GetLineHeight();
			
			float y = firstY;
			
			Vector4 shadowColor = {0, 0, 0, 0.5};
			
			for(it = entries.begin(); it != entries.end(); it++){
				ChatEntry& ent = *it;
				
				std::string msg = ent.msg;
				Vector4 color = GetColor(MsgColorRestore);
				float tx = 0.f, ty = y;
				float fade = ent.fade;
				if(ent.timeFade < 1.f)
					fade *= ent.timeFade;
				shadowColor.w = .5f * fade;
				color.w *= fade;
				for(size_t i = 0; i < msg.size(); i++){
					if(msg[i] == 13 || msg[i] == 10){
						tx = 0.f; ty += lHeight;
					}else if(msg[i] <= MsgColorMax){
						color = GetColor(msg[i]);
						color.w *= fade;
					}else{
						std::string ch(&msg[i], 1);
						font->Draw(ch,
								   MakeVector2(tx + winX + 1.f,
											   ty + winY + 1.f),
								   1.f, shadowColor);
						font->Draw(ch,
								   MakeVector2(tx + winX,
											   ty + winY),
								   1.f, color);
						tx += font->Measure(ch).x;
					}
				}
				
				y += ent.height;
			}
		}
	}
}

