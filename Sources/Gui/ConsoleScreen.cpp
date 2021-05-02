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
#include <ScriptBindings/Config.h>
#include <ScriptBindings/ScriptFunction.h>

#include <Client/Fonts.h>

#include "ConfigConsoleResponder.h"
#include "ConsoleCommand.h"
#include "ConsoleHelper.h"
#include "ConsoleScreen.h"

namespace spades {
	namespace gui {
		ConsoleScreen::ConsoleScreen(Handle<client::IRenderer> renderer,
		                             Handle<client::IAudioDevice> audioDevice,
		                             Handle<client::FontManager> fontManager, Handle<View> subview)
		    : renderer{renderer}, audioDevice{audioDevice}, subview{subview} {
			SPADES_MARK_FUNCTION();

			helper = Handle<ConsoleHelper>::New(this);

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction uiFactory("ConsoleUI@ CreateConsoleUI(Renderer@, "
			                                "AudioDevice@, FontManager@, ConsoleHelper@)");
			{
				ScriptContextHandle ctx = uiFactory.Prepare();
				ctx->SetArgObject(0, &*renderer);
				ctx->SetArgObject(1, &*audioDevice);
				ctx->SetArgObject(2, &*fontManager);
				ctx->SetArgObject(3, &*helper);

				ctx.ExecuteChecked();
				ui = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			}
		}

		ConsoleScreen::~ConsoleScreen() {
			SPADES_MARK_FUNCTION();

			helper->ConsoleScreenDestroyed();
		}

		void ConsoleScreen::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "void MouseEvent(float, float)");
				ScriptContextHandle c = func.Prepare();
				c->SetObject(&*ui);
				c->SetArgFloat(0, x);
				c->SetArgFloat(1, y);
				c.ExecuteChecked();
			} else {
				return subview->MouseEvent(x, y);
			}
		}
		void ConsoleScreen::KeyEvent(const std::string &key, bool down) {
			SPADES_MARK_FUNCTION();

			// TODO: Check if "`" is correct
			if (key == "`" || key == "F1") {
				if (down) {
					ToggleConsole();
				}
				return;
			}

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "void KeyEvent(string, bool)");
				ScriptContextHandle c = func.Prepare();
				std::string k = key;
				c->SetObject(&*ui);
				c->SetArgObject(0, reinterpret_cast<void *>(&k));
				c->SetArgByte(1, down ? 1 : 0);
				c.ExecuteChecked();
			} else {
				return subview->KeyEvent(key, down);
			}
		}
		void ConsoleScreen::TextInputEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "void TextInputEvent(string)");
				ScriptContextHandle c = func.Prepare();
				std::string k = ch;
				c->SetObject(&*ui);
				c->SetArgObject(0, reinterpret_cast<void *>(&k));
				c.ExecuteChecked();
			} else {
				return subview->TextInputEvent(ch);
			}
		}
		void ConsoleScreen::TextEditingEvent(const std::string &ch, int start, int len) {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "void TextEditingEvent(string, int, int)");
				ScriptContextHandle c = func.Prepare();
				std::string k = ch;
				c->SetObject(&*ui);
				c->SetArgObject(0, reinterpret_cast<void *>(&k));
				c->SetArgDWord(1, static_cast<asDWORD>(start));
				c->SetArgDWord(2, static_cast<asDWORD>(len));
				c.ExecuteChecked();
			} else {
				return subview->TextEditingEvent(ch, start, len);
			}
		}
		bool ConsoleScreen::AcceptsTextInput() {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "bool AcceptsTextInput()");
				ScriptContextHandle c = func.Prepare();
				c->SetObject(&*ui);
				c.ExecuteChecked();
				return c->GetReturnByte() != 0;
			} else {
				return subview->AcceptsTextInput();
			}
		}
		AABB2 ConsoleScreen::GetTextInputRect() {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "AABB2 GetTextInputRect()");
				ScriptContextHandle c = func.Prepare();
				c->SetObject(&*ui);
				c.ExecuteChecked();
				return *reinterpret_cast<AABB2 *>(c->GetReturnObject());
			} else {
				return subview->GetTextInputRect();
			}
		}
		void ConsoleScreen::WheelEvent(float x, float y) {
			SPADES_MARK_FUNCTION();

			if (ShouldInterceptInput()) {
				ScopedPrivilegeEscalation privilege;
				static ScriptFunction func("ConsoleUI", "void WheelEvent(float, float)");
				ScriptContextHandle c = func.Prepare();
				c->SetObject(&*ui);
				c->SetArgFloat(0, x);
				c->SetArgFloat(1, y);
				c.ExecuteChecked();
			} else {
				return subview->WheelEvent(x, y);
			}
		}
		bool ConsoleScreen::NeedsAbsoluteMouseCoordinate() {
			SPADES_MARK_FUNCTION();

			return ShouldInterceptInput() ? true : subview->NeedsAbsoluteMouseCoordinate();
		}

		void ConsoleScreen::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();

			// Read the global log buffer dedicated to `ConsoleScreen`
			GetBufferedLogLines(stmp::make_fn<void(std::string)>([this](std::string entry) {
				auto lines = SplitIntoLines(entry);
				for (const auto &line : lines) {
					AddLine(line);
				}
			}));

			subview->RunFrame(dt);

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ConsoleUI", "void RunFrame(float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, dt);
			c.ExecuteChecked();
		}

		void ConsoleScreen::RunFrameLate(float dt) {
			SPADES_MARK_FUNCTION();
			subview->RunFrameLate(dt);
		}

		void ConsoleScreen::Closing() {
			SPADES_MARK_FUNCTION();
			subview->Closing();
		}

		bool ConsoleScreen::WantsToBeClosed() {
			SPADES_MARK_FUNCTION();
			return subview->WantsToBeClosed();
		}

		void ConsoleScreen::DumpAllCommands() {
			SPADES_MARK_FUNCTION();
			auto it = AutocompleteCommandName("");
			while (it->MoveNext()) {
				const auto &cmd = it->GetCurrent();
				SPLog("%s %s", cmd.name.c_str(), cmd.description.c_str());
			}
		}

		namespace {
			constexpr const char *CMD_HELP = "help";
			constexpr const char *CMD_CLEARGFXCACHE = "cleargfxcache";
			constexpr const char *CMD_CLEARSFXCACHE = "clearsfxcache";

			std::map<std::string, std::string> const g_commands{
			  {CMD_HELP, ": Display all available commands"},
			  {CMD_CLEARGFXCACHE, ": Clear the GFX (models and images) cache, forcing reload"},
			  {CMD_CLEARSFXCACHE, ": Clear the SFX cache, forcing reload"},
			};
		} // namespace

		bool ConsoleScreen::ExecCommand(const Handle<ConsoleCommand> &command) {
			SPADES_MARK_FUNCTION();
			if (command->GetName() == CMD_HELP) {
				if (command->GetNumArguments() != 0) {
					SPLog("Usage: %s (no arguments)", CMD_HELP);
					return true;
				}
				DumpAllCommands();
				return true;
			} else if (command->GetName() == CMD_CLEARGFXCACHE) {
				if (command->GetNumArguments() != 0) {
					SPLog("Usage: %s (no arguments)", CMD_CLEARGFXCACHE);
					return true;
				}
				renderer->ClearCache();
				return true;
			} else if (command->GetName() == CMD_CLEARSFXCACHE) {
				if (command->GetNumArguments() != 0) {
					SPLog("Usage: %s (no arguments)", CMD_CLEARSFXCACHE);
					return true;
				}
				audioDevice->ClearCache();
				return true;
			}
			return ConfigConsoleResponder::ExecCommand(command) || subview->ExecCommand(command);
		}

		Handle<ConsoleCommandCandidateIterator>
		ConsoleScreen::AutocompleteCommandName(const std::string &name) {
			SPADES_MARK_FUNCTION();
			return MakeCandidates(g_commands, name) +
			       ConfigConsoleResponder::AutocompleteCommandName(name) +
			       subview->AutocompleteCommandName(name);
		}

		bool ConsoleScreen::ShouldInterceptInput() {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ConsoleUI", "bool ShouldInterceptInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		void ConsoleScreen::ToggleConsole() {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ConsoleUI", "void ToggleConsole()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}

		void ConsoleScreen::AddLine(const std::string &line) {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ConsoleUI", "void AddLine(string)");
			ScriptContextHandle c = func.Prepare();
			std::string k = line;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c.ExecuteChecked();
		}
	} // namespace gui
} // namespace spades
