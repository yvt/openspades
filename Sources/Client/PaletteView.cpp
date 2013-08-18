//
//  PaletteView.cpp
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "PaletteView.h"
#include "Client.h"
#include "IRenderer.h"
#include "IImage.h"
#include "Player.h"
#include "World.h"
#include "NetClient.h"

namespace spades {
	namespace client {
		PaletteView::PaletteView(Client *client):
		client(client),
		renderer(client->GetRenderer()){
			IntVector3 cols[] = {
				{112, 112, 112},
				{224, 0, 0},
				{224, 112, 0},
				{224, 224, 0},
				{0, 224, 0},
				{0, 224, 224},
				{0, 0, 224},
				{224, 0, 224}
			};
			
			for(int i = 0; i < 8; i++) {
				colors.push_back(cols[i] / 7);
				colors.push_back(cols[i] / 112 * 27);
				colors.push_back(cols[i] * 3 / 7);
				colors.push_back(cols[i]);
				
				IntVector3 rem = IntVector3::Make(255, 255, 255);
				rem -= cols[i];
				
				colors.push_back(cols[i] + rem / 4);
				colors.push_back(cols[i] + rem * 85 / 255);
				colors.push_back(cols[i] + rem / 2);
				
				switch(i){
					case 0:
						colors.push_back(IntVector3::Make(240, 240, 240));
						break;
					case 1:
						colors.push_back(IntVector3::Make(224, 255, 255));
						break;
					case 2:
						colors.push_back(IntVector3::Make(224, 240, 255));
						break;
					case 3:
						colors.push_back(IntVector3::Make(224, 224, 255));
						break;
					case 4:
						colors.push_back(IntVector3::Make(255, 224, 255));
						break;
					case 5:
						colors.push_back(IntVector3::Make(255, 224, 224));
						break;
					case 6:
						colors.push_back(IntVector3::Make(255, 255, 224));
						break;
					case 7:
						colors.push_back(IntVector3::Make(224, 255, 224));
						break;
				}
			}
			
			defaultColor = 3;
		}
		
		PaletteView::~PaletteView() {
			
		}
		
		int PaletteView::GetSelectedIndex() {
			World *w = client->GetWorld();
			if(!w) return -1;
			
			Player *p = w->GetLocalPlayer();
			if(!p) return -1;
			
			IntVector3 col = p->GetBlockColor();
			for(int i = 0; i < (int)colors.size(); i++){
				if(col.x == colors[i].x &&
				   col.y == colors[i].y &&
				   col.z == colors[i].z)
					return i;
			}
			return -1;
		}
		
		int PaletteView::GetSelectedOrDefaultIndex() {
			int c = GetSelectedIndex();
			if(c == -1)
				return defaultColor;
			else
				return c;
		}
		
		void PaletteView::SetSelectedIndex(int idx) {
			IntVector3 col = colors[idx];
			
			World *w = client->GetWorld();
			if(!w) return;
			
			Player *p = w->GetLocalPlayer();
			if(!p) return;
			
			p->SetHeldBlockColor(col);
			
			client->net->SendHeldBlockColor();
		}
		
		bool PaletteView::KeyInput(std::string keyName) {
			if(keyName == "Left") {
				int c = GetSelectedOrDefaultIndex();
				if(c == 0)
					c = (int)colors.size() - 1;
				else
					c--;
				SetSelectedIndex(c);
				return true;
			}else if(keyName == "Right") {
				int c = GetSelectedOrDefaultIndex();
				if(c == (int)colors.size() - 1)
					c = 0;
				else
					c++;
				SetSelectedIndex(c);
				return true;
			}else if(keyName == "Up") {
				int c = GetSelectedOrDefaultIndex();
				if(c < 8)
					c += (int)colors.size() - 8;
				else
					c -= 8;
				SetSelectedIndex(c);
				return true;
			}else if(keyName == "Down") {
				int c = GetSelectedOrDefaultIndex();
				if(c >= (int)colors.size() - 8)
					c -= (int)colors.size() - 8;
				else
					c += 8;
				SetSelectedIndex(c);
				return true;
			}else{
				return false;
			}
		}
		
		void PaletteView::Update() {
			
		}
		
		void PaletteView::Draw() {
			IImage *img = renderer->RegisterImage("Gfx/Palette.png");
			
			int sel = GetSelectedIndex();
			
			float scrW = renderer->ScreenWidth();
			float scrH = renderer->ScreenHeight();
			
			for(int phase = 0; phase < 2; phase++){
				for(int i = 0; i < colors.size(); i++){
					if((sel == i) != (phase == 1))
						continue;
					
					int row = i / 8;
					int col = i % 8;
					
					bool selected = sel == i;
					
					// draw color
					IntVector3 icol = colors[i];
					Vector4 cl;
					cl.x = icol.x / 255.f;
					cl.y = icol.y / 255.f;
					cl.z = icol.z / 255.f;
					cl.w = 1.f;
					
					float x = scrW - 100.f + 10.f * col;
					float y = scrH - 96.f + 10.f * row - 40.f;
					
					renderer->SetColor(cl);
					if(selected){
						renderer->DrawImage(img,
											MakeVector2(x, y),
											AABB2(0, 16, 16, 16));
					}else{
						renderer->DrawImage(img,
											MakeVector2(x, y),
											AABB2(0, 0, 16, 16));
					}
					
					renderer->SetColor(MakeVector4(1, 1, 1, 1));
					if(selected){
						renderer->DrawImage(img,
											MakeVector2(x, y),
											AABB2(16, 16, 16, 16));
					}else{
						renderer->DrawImage(img,
											MakeVector2(x, y),
											AABB2(16, 0, 16, 16));
					}
				}
			}
		}
	}
}


