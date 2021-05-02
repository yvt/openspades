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

#pragma once

#include "View.h"
#include <Client/IAudioDevice.h>
#include <Client/IRenderer.h>
#include <Core/RefCountedObject.h>
#include <Core/ServerAddress.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		class FontManager;
	}
	namespace gui {
		class MainScreenHelper;
		class MainScreen : public View {
			friend class MainScreenHelper;
			Handle<client::IRenderer> renderer;
			Handle<client::IAudioDevice> audioDevice;
			Handle<View> subview;
			Handle<client::FontManager> fontManager;
			float timeToStartInitialization;

			Handle<MainScreenHelper> helper;
			Handle<asIScriptObject> ui;

			void DrawStartupScreen();
			void DoInit();

			void RestoreRenderer();

			std::string Connect(const ServerAddress &host);

		protected:
			~MainScreen();

		public:
			MainScreen(Handle<client::IRenderer>, Handle<client::IAudioDevice>,
			           Handle<client::FontManager>);

			client::IRenderer *GetRenderer() { return &*renderer; }
			client::IAudioDevice *GetAudioDevice() { return &*audioDevice; }

			void MouseEvent(float x, float y) override;
			void KeyEvent(const std::string &, bool down) override;
			void TextInputEvent(const std::string &) override;
			void TextEditingEvent(const std::string &, int start, int len) override;
			bool AcceptsTextInput() override;
			AABB2 GetTextInputRect() override;
			void WheelEvent(float x, float y) override;
			bool NeedsAbsoluteMouseCoordinate() override;

			void RunFrame(float dt) override;
			void RunFrameLate(float dt) override;

			void Closing() override;
			bool WantsToBeClosed() override;

			bool ExecCommand(const Handle<ConsoleCommand> &) override;
			Handle<ConsoleCommandCandidateIterator>
			AutocompleteCommandName(const std::string &name) override;
		};
	} // namespace gui
} // namespace spades
