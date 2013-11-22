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


#include "MainScreen.h"
#include <Core/Exception.h>
#include <Client/Quake3Font.h>
#include <Client/FontData.h>

namespace spades {
	namespace gui {
		MainScreen::MainScreen(client::IRenderer *r, client::IAudioDevice *a) :
		renderer(r),
		audioDevice(a){
			if(r == NULL)
				SPInvalidArgument("r");
			if(a == NULL)
				SPInvalidArgument("a");
			
			
			font = new client::Quake3Font(renderer,
									  renderer->RegisterImage("Gfx/Fonts/UbuntuCondensed.tga"),
									  (const int*)UbuntuCondensedMap,
									  24,
									  4);
			SPLog("Font 'Ubuntu Condensed' Loaded");
			
			shouldBeClosed = false;
		}
		
		MainScreen::~MainScreen(){
			
		}
		
		void MainScreen::MouseEvent(float x, float y) {
			if(subview){
				subview->MouseEvent(x, y);
				return;
			}
		}
		
		void MainScreen::KeyEvent(const std::string & key, bool down) {
			if(subview){
				subview->KeyEvent(key, down);
				return;
			}
			if(key == "Escape"){
				shouldBeClosed = true;
			}
		}
		
		void MainScreen::CharEvent(const std::string &ch) {
			if(subview){
				subview->CharEvent(ch);
				return;
			}
		}
		
		void MainScreen::RunFrame(float dt) {
			if(subview){
				subview->RunFrame(dt);
				return;
			}
			float w = renderer->ScreenWidth();
			float h = renderer->ScreenHeight();
			
			Handle<client::IImage> img = renderer->RegisterImage("Gfx/White.tga");
			renderer->SetColor(MakeVector4(0.f, 0.f, 0.f, 1.f));
			renderer->DrawImage(img, AABB2(0.f, 0.f, w, h));
			font->Draw("Not Implemented", MakeVector2(20.f, 20.f),
					   1.f, MakeVector4(1.f, 1.f, 1.f, 1.f));
			
			renderer->FrameDone();
			renderer->Flip();
		}
		
		void MainScreen::Closing() {
			if(subview){
				subview->Closing();
				subview = NULL;
			}
		}
	}
}

