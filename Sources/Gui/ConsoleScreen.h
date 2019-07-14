/*
 Copyright (c) 2019 yvt

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
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		class FontManager;
	}
	namespace gui {
		class ConsoleHelper;

		/**
		 * This `View` wraps another `View` to provide the system console
		 * functionality which can be invoked anytime by using a hotkey.
		 */
		class ConsoleScreen : public View {
			friend class ConsoleHelper;
		public:
			ConsoleScreen(Handle<client::IRenderer>, Handle<client::IAudioDevice>,
			              Handle<client::FontManager>, Handle<View>);

			// Implements `View`
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

		private:
			~ConsoleScreen();

			Handle<client::IRenderer> renderer;
			Handle<client::IAudioDevice> audioDevice;
			Handle<View> subview;

			// Scripting
			Handle<ConsoleHelper> helper;
			Handle<asIScriptObject> ui;
			bool ShouldInterceptInput();
			void ToggleConsole();
			void AddLine(const std::string &);

			/** Dump all available commands to `SPLog`. */
			void DumpAllCommands();
		};
	} // namespace gui
} // namespace spades
