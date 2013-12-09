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
#include <ScriptBindings/ScriptFunction.h>
#include "MainScreenHelper.h"
#include <Client/Client.h>
#include <Core/Settings.h>

SPADES_SETTING(cg_lastQuickConnectHost, "127.0.0.1");
SPADES_SETTING(cg_protocolVersion, "");
SPADES_SETTING(cg_playerName, "Deuce");

namespace spades {
	namespace gui {
		MainScreen::MainScreen(client::IRenderer *r, client::IAudioDevice *a) :
		renderer(r),
		audioDevice(a){
			SPADES_MARK_FUNCTION();
			if(r == NULL)
				SPInvalidArgument("r");
			if(a == NULL)
				SPInvalidArgument("a");
			
			
			font = new client::Quake3Font(renderer,
									  renderer->RegisterImage("Gfx/Fonts/SquareFontModified.png"),
									  (const int*)SquareFontMap,
									  24,
									  4);
			static_cast<client::Quake3Font*>(&*font)->SetGlyphYRange(4.f, 16.f);
			SPLog("Font 'SquareFont' Loaded");
			
			helper = new MainScreenHelper(this);
						
			// first call to RunFrame tends to have larger dt value.
			// so this value is set in the first call.
			timeToStartInitialization = 1000.f;
		}
		
		MainScreen::~MainScreen(){
			SPADES_MARK_FUNCTION();
			helper->MainScreenDestroyed();
		}
		
		// Restores renderer's state (game map, fog color)
		// after returning from the game client.
		void MainScreen::RestoreRenderer() {
			static ScriptFunction func("MainScreenUI", "void SetupRenderer()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		
		void MainScreen::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->MouseEvent(x, y);
				return;
			}
			if(!ui){
				return;
			}
			
			static ScriptFunction func("MainScreenUI", "void MouseEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}
		
		
		void MainScreen::WheelEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->WheelEvent(x, y);
				return;
			}
			if(!ui){
				return;
			}
			
			static ScriptFunction func("MainScreenUI", "void WheelEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}
		
		void MainScreen::KeyEvent(const std::string & key, bool down) {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->KeyEvent(key, down);
				return;
			}
			if(!ui){
				return;
			}
			static ScriptFunction func("MainScreenUI", "void KeyEvent(string, bool)");
			ScriptContextHandle c = func.Prepare();
			std::string k = key;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void*>(&k));
			c->SetArgByte(1, down ? 1 : 0);
			c.ExecuteChecked();
		}
		
		void MainScreen::TextInputEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->TextInputEvent(ch);
				return;
			}
			if(!ui){
				return;
			}
			static ScriptFunction func("MainScreenUI", "void TextInputEvent(string)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void*>(&k));
			c.ExecuteChecked();
		}
		
		void MainScreen::TextEditingEvent(const std::string &ch,
										  int start, int len) {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->TextEditingEvent(ch, start, len);
				return;
			}
			if(!ui){
				return;
			}
			static ScriptFunction func("MainScreenUI", "void TextEditingEvent(string, int, int)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void*>(&k));
			c->SetArgDWord(1, static_cast<asDWORD>(start));
			c->SetArgDWord(2, static_cast<asDWORD>(len));
			c.ExecuteChecked();
		}
		
		bool MainScreen::AcceptsTextInput() {
			SPADES_MARK_FUNCTION();
			if(subview){
				return subview->AcceptsTextInput();
			}
			if(!ui){
				return false;
			}
			static ScriptFunction func("MainScreenUI", "bool AcceptsTextInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}
		
		AABB2 MainScreen::GetTextInputRect() {
			SPADES_MARK_FUNCTION();
			if(subview){
				return subview->GetTextInputRect();
			}
			if(!ui){
				return AABB2();
			}
			static ScriptFunction func("MainScreenUI", "AABB2 GetTextInputRect()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return *reinterpret_cast<AABB2*>(c->GetReturnObject());
		}
		
		bool MainScreen::WantsToBeClosed() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return false;
			}
			static ScriptFunction func("MainScreenUI", "bool WantsToBeClosed()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}
		
		void MainScreen::DrawStartupScreen() {
			SPADES_MARK_FUNCTION();
			Handle<client::IImage> img;
			Vector2 scrSize = {renderer->ScreenWidth(),
				renderer->ScreenHeight()};
			
			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 1.));
			img = renderer->RegisterImage("Gfx/White.tga");
			renderer->DrawImage(img, AABB2(0, 0,
										   scrSize.x, scrSize.y));
			
			std::string str = "NOW LOADING";
			Vector2 size = font->Measure(str);
			Vector2 pos = MakeVector2(scrSize.x - 16.f, scrSize.y - 16.f);
			pos -= size;
			font->DrawShadow(str, pos, 1.f, MakeVector4(1,1,1,1), MakeVector4(0,0,0,0.5));
			
			renderer->FrameDone();
			renderer->Flip();
		}
		
		void MainScreen::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();
			if(subview){
				try{
					subview->RunFrame(dt);
					if(subview->WantsToBeClosed()) {
						subview->Closing();
						subview = NULL;
						RestoreRenderer();
					}else{
						return;
					}
				}catch(const std::exception& ex) {
					subview->Closing();
					subview = NULL;
					RestoreRenderer();
					helper->errorMessage = ex.what();
				}
			}
			if(timeToStartInitialization > 100.f){
				timeToStartInitialization = 0.2f;
			}
			if(timeToStartInitialization > 0.f){
				DrawStartupScreen();
				timeToStartInitialization -= dt;
				if(timeToStartInitialization <= 0.f){
					// do init
					DoInit();
				}
				return;
			}
			
			static ScriptFunction func("MainScreenUI", "void RunFrame(float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, dt);
			c.ExecuteChecked();
		}
		
		void MainScreen::DoInit() {
			SPADES_MARK_FUNCTION();
			renderer->Init();
			
			static ScriptFunction uiFactory("MainScreenUI@ CreateMainScreenUI(Renderer@, AudioDevice@, Font@, MainScreenHelper@)");
			{
				ScriptContextHandle ctx = uiFactory.Prepare();
				ctx->SetArgObject(0, renderer);
				ctx->SetArgObject(1, audioDevice);
				ctx->SetArgObject(2, font);
				ctx->SetArgObject(3, &*helper);
				
				ctx.ExecuteChecked();
				ui = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			}

		}
		
		void MainScreen::Closing() {
			SPADES_MARK_FUNCTION();
			if(subview){
				subview->Closing();
				subview = NULL;
			}
			
			if(!ui){
				return;
			}
			static ScriptFunction func("MainScreenUI", "void Closing()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		
		std::string MainScreen::Connect() {
			spades::ServerAddress host(cg_lastQuickConnectHost, (int)cg_protocolVersion == 3 ? spades::ProtocolVersion::v075 : spades::ProtocolVersion::v076);
			try {
				subview.Set(new client::Client(&*renderer, &*audioDevice,
											 host, cg_playerName), false);
			}catch(const std::exception& ex) {
				return ex.what();
			}
			return "";
		}
	}
}

