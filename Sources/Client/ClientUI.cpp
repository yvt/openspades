/*
 Copyright (c) 2013 yvt
 Portion of the code is based on Serverbrowser.cpp.
 
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


#include "ClientUI.h"
#include <Core/Exception.h>
#include <Client/Quake3Font.h>
#include <Client/FontData.h>
#include <ScriptBindings/ScriptFunction.h>
#include "ClientUIHelper.h"
#include <Client/Client.h>
#include <Core/Settings.h>
#include "NetClient.h"

namespace spades {
	namespace client {
		ClientUI::ClientUI(client::IRenderer *r, client::IAudioDevice *a, IFont *font, Client *client) :
		renderer(r),
		audioDevice(a),
		font(font),client(client){
			SPADES_MARK_FUNCTION();
			if(r == NULL)
				SPInvalidArgument("r");
			if(a == NULL)
				SPInvalidArgument("a");
			
			helper.Set(new ClientUIHelper(this), false);
			
			static ScriptFunction uiFactory("ClientUI@ CreateClientUI(Renderer@, AudioDevice@, Font@, ClientUIHelper@)");
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
		
		ClientUI::~ClientUI(){
			SPADES_MARK_FUNCTION();
			helper->ClientUIDestroyed();
		}
		
		void ClientUI::SendChat(const std::string &msg,
								bool isGlobal) {
			if(!client) return;
			client->net->SendChat(msg, isGlobal);
		}
		
		void ClientUI::ClientDestroyed() {
			client = NULL;
		}
		
		void ClientUI::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			
			static ScriptFunction func("ClientUI", "void MouseEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}
		
		void ClientUI::KeyEvent(const std::string & key, bool down) {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void KeyEvent(string, bool)");
			ScriptContextHandle c = func.Prepare();
			std::string k = key;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void*>(&k));
			c->SetArgByte(1, down ? 1 : 0);
			c.ExecuteChecked();
		}
		
		void ClientUI::CharEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void CharEvent(string)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void*>(&k));
			c.ExecuteChecked();
		}
		
		bool ClientUI::WantsClientToBeClosed() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return false;
			}
			static ScriptFunction func("ClientUI", "bool WantsClientToBeClosed()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}
		
		bool ClientUI::NeedsInput() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return false;
			}
			static ScriptFunction func("ClientUI", "bool NeedsInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}
				
		void ClientUI::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();
			static ScriptFunction func("ClientUI", "void RunFrame(float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, dt);
			c.ExecuteChecked();
		}
		
		void ClientUI::Closing() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void Closing()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		
		void ClientUI::EnterClientMenu() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void EnterClientMenu()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterGlobalChatWindow() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void EnterGlobalChatWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterTeamChatWindow() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void EnterTeamChatWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterCommandWindow() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void EnterCommandWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::CloseUI() {
			SPADES_MARK_FUNCTION();
			if(!ui){
				return;
			}
			static ScriptFunction func("ClientUI", "void CloseUI()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		
	}
}

