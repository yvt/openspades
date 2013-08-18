//
//  CenterMessageView.cpp
//  OpenSpades
//
//  Created by yvt on 7/19/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "CenterMessageView.h"
#include "Client.h"
#include "IRenderer.h"
#include "IFont.h"
#include "../Core/Debug.h"

namespace spades {
	namespace client {
		CenterMessageView::CenterMessageView(Client *client,
											 IFont *font):
		client(client), font(font), renderer(client->GetRenderer()){
			SPADES_MARK_FUNCTION();
			
			for(int i = 0; i < 5; i++)
				lineUsing.push_back(false);
			
		}
		CenterMessageView::~CenterMessageView() {
			
		}
		
		int CenterMessageView::GetFreeLine(){
			for(size_t i = 0; i < lineUsing.size(); i++)
				if(!lineUsing[i])
					return (int)i;
			
			// remove oldest entry
			int l = entries.front().line;
			entries.pop_front();
			return l;
		}
		
		void CenterMessageView::AddMessage(const std::string &msg) {
			SPADES_MARK_FUNCTION();
			
			Entry entry;
			entry.msg = msg;
			entry.fade = 10.f;
			entry.line = GetFreeLine();
			lineUsing[entry.line] = true;
			entries.push_back(entry);
		}
		
		
		
		void CenterMessageView::Update(float dt) {
			SPADES_MARK_FUNCTION();
			
			std::list<Entry>::iterator it;
			std::vector<std::list<Entry>::iterator> its;
			for(it = entries.begin(); it != entries.end(); it++){
				Entry& ent = *it;
				ent.fade -= dt;
				if(ent.fade < 0){
					lineUsing[ent.line] = false;
					its.push_back(it);
				}
			}
			for(size_t i = 0; i < its.size(); i++)
				entries.erase(its[i]);
		}
		
		void CenterMessageView::Draw() {
			SPADES_MARK_FUNCTION();
			
			std::list<Entry>::iterator it;
			for(it = entries.begin(); it != entries.end(); it++){
				Entry& ent = *it;
				
				Vector2 size = font->Measure(ent.msg);
				float fade = ent.fade;
				if(fade > 1.f)
					fade = 1.f;
				
				float y = 100.f + 32.f * (float)ent.line;
				float x = (renderer->ScreenWidth() - size.x) * .5f;
				
				Vector4 shadow = {0, 0, 0, fade * 0.5f};
				Vector4 color = {1, 1, 1, fade};
				
				font->Draw(ent.msg, MakeVector2(x + 1, y + 1),
						   1.f, shadow);
				font->Draw(ent.msg, MakeVector2(x, y),
						   1.f, color);
			}
		}
	}
}
