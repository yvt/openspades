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
#include "ClientUIHelper.h"
#include "NetClient.h"
#include <Client/Client.h>
#include <Client/FontData.h>
#include <Client/Fonts.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include <ScriptBindings/Config.h>
#include <ScriptBindings/ScriptFunction.h>

namespace spades {
	namespace client {
		ClientUI::ClientUI(IRenderer *r, IAudioDevice *a, FontManager *fontManager, Client *client)
		    : renderer(r), audioDevice(a), fontManager(fontManager), client(client) {
			SPADES_MARK_FUNCTION();
			if (!renderer)
				SPInvalidArgument("renderer");
			if (!audioDevice)
				SPInvalidArgument("audioDevice");

			helper = Handle<ClientUIHelper>::New(this);

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction uiFactory(
			  "ClientUI@ CreateClientUI(Renderer@, AudioDevice@, FontManager@, ClientUIHelper@)");
			{
				ScriptContextHandle ctx = uiFactory.Prepare();
				ctx->SetArgObject(0, renderer.GetPointerOrNull());
				ctx->SetArgObject(1, audioDevice.GetPointerOrNull());
				ctx->SetArgObject(2, fontManager);
				ctx->SetArgObject(3, &*helper);

				ctx.ExecuteChecked();
				ui = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			}
		}

		ClientUI::~ClientUI() {
			SPADES_MARK_FUNCTION();
			helper->ClientUIDestroyed();
		}

		void ClientUI::SendChat(const std::string &msg, bool isGlobal) {
			if (!client)
				return;
			client->net->SendChat(msg, isGlobal);
		}

		void ClientUI::AlertNotice(const std::string &msg) {
			if (!client)
				return;
			client->ShowAlert(msg, Client::AlertType::Notice);
		}

		void ClientUI::AlertWarning(const std::string &msg) {
			if (!client)
				return;
			client->ShowAlert(msg, Client::AlertType::Warning);
		}

		void ClientUI::AlertError(const std::string &msg) {
			if (!client)
				return;
			client->ShowAlert(msg, Client::AlertType::Error);
		}

		void ClientUI::ClientDestroyed() { client = NULL; }

		void ClientUI::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void MouseEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}

		void ClientUI::WheelEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void WheelEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}

		void ClientUI::KeyEvent(const std::string &key, bool down) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void KeyEvent(string, bool)");
			ScriptContextHandle c = func.Prepare();
			std::string k = key;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c->SetArgByte(1, down ? 1 : 0);
			c.ExecuteChecked();
		}

		void ClientUI::TextInputEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void TextInputEvent(string)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c.ExecuteChecked();
		}

		void ClientUI::TextEditingEvent(const std::string &ch, int start, int len) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void TextEditingEvent(string,int,int)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c->SetArgDWord(1, static_cast<asDWORD>(start));
			c->SetArgDWord(2, static_cast<asDWORD>(len));
			c.ExecuteChecked();
		}

		bool ClientUI::AcceptsTextInput() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return false;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "bool AcceptsTextInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		AABB2 ClientUI::GetTextInputRect() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return AABB2();
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "AABB2 GetTextInputRect()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return *reinterpret_cast<AABB2 *>(c->GetReturnObject());
		}

		bool ClientUI::WantsClientToBeClosed() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return false;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "bool WantsClientToBeClosed()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		bool ClientUI::NeedsInput() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return false;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "bool NeedsInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		void ClientUI::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void RunFrame(float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, dt);
			c.ExecuteChecked();
		}

		void ClientUI::Closing() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void Closing()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}

		void ClientUI::RecordChatLog(const std::string &msg, Vector4 color) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void RecordChatLog(string, Vector4)");
			ScriptContextHandle c = func.Prepare();
			std::string k = msg;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c->SetArgObject(1, reinterpret_cast<void *>(&color));
			c.ExecuteChecked();
		}

		void ClientUI::EnterClientMenu() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void EnterClientMenu()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterGlobalChatWindow() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void EnterGlobalChatWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterTeamChatWindow() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void EnterTeamChatWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::EnterCommandWindow() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void EnterCommandWindow()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}
		void ClientUI::CloseUI() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("ClientUI", "void CloseUI()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}

		bool ClientUI::isIgnored(const std::string &key) {
			return !ignoreInput.empty() && EqualsIgnoringCase(ignoreInput, key);
		}
		void ClientUI::setIgnored(const std::string &key) { ignoreInput = key; }
	} // namespace client
} // namespace spades
