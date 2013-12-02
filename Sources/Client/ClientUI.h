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
#pragma once


#include <Core/RefCountedObject.h>
#include <Client/IRenderer.h>
#include <Client/IAudioDevice.h>
#include <Gui/View.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		class IFont;
		class Client;
		class ClientUIHelper;
		class ClientUI: public RefCountedObject {
			friend class ClientUIHelper;
			Handle<client::IRenderer> renderer;
			Handle<client::IAudioDevice> audioDevice;
			Handle<client::IFont> font;
			
			Handle<ClientUIHelper> helper;
			Handle<asIScriptObject> ui;
			
			// weak reference
			Client *client;
			
			void SendChat(const std::string&, bool isGlobal);
			
		protected:
			virtual ~ClientUI();
		public:
			ClientUI(client::IRenderer *, client::IAudioDevice *, client::IFont *font, Client *client);
			void ClientDestroyed();
			
			client::IRenderer *GetRenderer() { return &*renderer; }
			client::IAudioDevice *GetAudioDevice() { return &*audioDevice; }
			
			void MouseEvent(float x, float y);
			void KeyEvent(const std::string&,
								  bool down);
			void CharEvent(const std::string&);
			
			void RunFrame(float dt);
			
			void Closing();
			
			bool WantsClientToBeClosed();
			
			bool NeedsInput();
			
			void EnterClientMenu();
			void EnterGlobalChatWindow();
			void EnterTeamChatWindow();
			void EnterCommandWindow();
			void CloseUI();
		};;
	}
}
