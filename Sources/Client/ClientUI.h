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

#include <Client/IAudioDevice.h>
#include <Client/IRenderer.h>
#include <Core/RefCountedObject.h>
#include <Gui/View.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		class FontManager;
		class Client;
		class ClientUIHelper;
		class ClientUI : public RefCountedObject {
			friend class ClientUIHelper;
			Handle<IRenderer> renderer;
			Handle<IAudioDevice> audioDevice;
			Handle<FontManager> fontManager;

			Handle<ClientUIHelper> helper;
			Handle<asIScriptObject> ui;

			// weak reference
			Client *client;
			std::string ignoreInput;

			void SendChat(const std::string &, bool isGlobal);

			void AlertNotice(const std::string &);
			void AlertWarning(const std::string &);
			void AlertError(const std::string &);

		protected:
			~ClientUI();

		public:
			ClientUI(IRenderer *, IAudioDevice *, FontManager *font, Client *client);
			void ClientDestroyed();

			client::IRenderer *GetRenderer() { return &*renderer; }
			client::IAudioDevice *GetAudioDevice() { return &*audioDevice; }

			void MouseEvent(float x, float y);
			void WheelEvent(float x, float y);
			void KeyEvent(const std::string &, bool down);
			void TextInputEvent(const std::string &);
			void TextEditingEvent(const std::string &, int start, int len);
			bool AcceptsTextInput();
			AABB2 GetTextInputRect();

			void RunFrame(float dt);

			void Closing();

			bool WantsClientToBeClosed();

			bool NeedsInput();

			void RecordChatLog(const std::string &, Vector4 color);

			void EnterClientMenu();
			void EnterGlobalChatWindow();
			void EnterTeamChatWindow();
			void EnterCommandWindow();
			void CloseUI();

			// lm: so the chat does not have the initial chat key
			bool isIgnored(const std::string &key);
			void setIgnored(const std::string &key);
		};
		;
	}
}
