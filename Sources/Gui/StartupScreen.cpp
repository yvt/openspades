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

#include "StartupScreen.h"

#include "StartupScreenHelper.h"
#include <Audio/NullDevice.h>
#include <Client/Client.h>
#include <Client/Fonts.h>
#include <Client/Quake3Font.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include <Gui/Main.h>
#include <Gui/SDLRunner.h>
#include <ScriptBindings/Config.h>
#include <ScriptBindings/ScriptFunction.h>

namespace spades {
	namespace gui {
		StartupScreen::StartupScreen(client::IRenderer *r, client::IAudioDevice *a,
		                             StartupScreenHelper *helper, client::FontManager *fontManager)
		    : renderer(r),
		      audioDevice(a),
		      fontManager(fontManager),
		      startRequested(false),
		      helper(helper) {
			SPADES_MARK_FUNCTION();
			if (r == NULL)
				SPInvalidArgument("r");
			if (a == NULL)
				SPInvalidArgument("a");

			helper->BindStartupScreen(this);

			DoInit();
		}

		StartupScreen::~StartupScreen() {
			SPADES_MARK_FUNCTION();
			helper->StartupScreenDestroyed();
		}

		// Restores renderer's state (game map, fog color)
		// after returning from the game client.
		void StartupScreen::RestoreRenderer() {
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void SetupRenderer()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}

		bool StartupScreen::NeedsAbsoluteMouseCoordinate() {
			SPADES_MARK_FUNCTION();
			return true;
		}

		void StartupScreen::MouseEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void MouseEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}

		void StartupScreen::WheelEvent(float x, float y) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void WheelEvent(float, float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, x);
			c->SetArgFloat(1, y);
			c.ExecuteChecked();
		}

		void StartupScreen::KeyEvent(const std::string &key, bool down) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void KeyEvent(string, bool)");
			ScriptContextHandle c = func.Prepare();
			std::string k = key;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c->SetArgByte(1, down ? 1 : 0);
			c.ExecuteChecked();
		}

		void StartupScreen::TextInputEvent(const std::string &ch) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void TextInputEvent(string)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c.ExecuteChecked();
		}

		void StartupScreen::TextEditingEvent(const std::string &ch, int start, int len) {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI",
			                           "void TextEditingEvent(string, int, int)");
			ScriptContextHandle c = func.Prepare();
			std::string k = ch;
			c->SetObject(&*ui);
			c->SetArgObject(0, reinterpret_cast<void *>(&k));
			c->SetArgDWord(1, static_cast<asDWORD>(start));
			c->SetArgDWord(2, static_cast<asDWORD>(len));
			c.ExecuteChecked();
		}

		bool StartupScreen::AcceptsTextInput() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return false;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "bool AcceptsTextInput()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		AABB2 StartupScreen::GetTextInputRect() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return AABB2();
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "AABB2 GetTextInputRect()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return *reinterpret_cast<AABB2 *>(c->GetReturnObject());
		}

		bool StartupScreen::WantsToBeClosed() {
			SPADES_MARK_FUNCTION();
			if (!ui) {
				return false;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "bool WantsToBeClosed()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
			return c->GetReturnByte() != 0;
		}

		void StartupScreen::DrawStartupScreen() {
			SPADES_MARK_FUNCTION();

			// FIXME: not used
			Handle<client::IImage> img;
			Vector2 scrSize = {renderer->ScreenWidth(), renderer->ScreenHeight()};

			renderer->SetColorAlphaPremultiplied(MakeVector4(0, 0, 0, 1.));
			img = renderer->RegisterImage("Gfx/White.tga");
			renderer->DrawImage(img, AABB2(0, 0, scrSize.x, scrSize.y));

			std::string str = "NOW LOADING";
			Vector2 size = fontManager->GetGuiFont()->Measure(str);
			Vector2 pos = MakeVector2(scrSize.x - 16.f, scrSize.y - 16.f);
			pos -= size;
			fontManager->GetGuiFont()->DrawShadow(str, pos, 1.f, MakeVector4(1, 1, 1, 1),
			                                      MakeVector4(0, 0, 0, 0.5));

			renderer->FrameDone();
			renderer->Flip();
		}

		void StartupScreen::RunFrame(float dt) {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void RunFrame(float)");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c->SetArgFloat(0, dt);
			c.ExecuteChecked();
		}

		void StartupScreen::DoInit() {
			SPADES_MARK_FUNCTION();

			ScopedPrivilegeEscalation privilege;
			static ScriptFunction uiFactory("StartupScreenUI@ CreateStartupScreenUI(Renderer@, "
			                                "AudioDevice@, FontManager@, StartupScreenHelper@)");
			{
				ScriptContextHandle ctx = uiFactory.Prepare();
				ctx->SetArgObject(0, renderer);
				ctx->SetArgObject(1, audioDevice);
				ctx->SetArgObject(2, fontManager);
				ctx->SetArgObject(3, &*helper);

				ctx.ExecuteChecked();
				ui = reinterpret_cast<asIScriptObject *>(ctx->GetReturnObject());
			}
		}

		void StartupScreen::Closing() {
			SPADES_MARK_FUNCTION();

			if (!ui) {
				return;
			}
			ScopedPrivilegeEscalation privilege;
			static ScriptFunction func("StartupScreenUI", "void Closing()");
			ScriptContextHandle c = func.Prepare();
			c->SetObject(&*ui);
			c.ExecuteChecked();
		}

		void StartupScreen::Start() { startRequested = true; }

		void StartupScreen::Run() {

			SDL_InitSubSystem(SDL_INIT_VIDEO);

			auto *helper = new StartupScreenHelper();
			helper->ExamineSystem();

			class ConcreteRunner : public SDLRunner {
				Handle<StartupScreen> view;
				StartupScreenHelper *helper;

			protected:
				auto GetRendererType() -> RendererType override { return RendererType::SW; }
				client::IAudioDevice *CreateAudioDevice() override {
					return new audio::NullDevice();
				}
				View *CreateView(client::IRenderer *renderer, client::IAudioDevice *dev) override {
					Handle<client::FontManager> fontManager(new client::FontManager(renderer),
					                                        false);
					view.Set(new StartupScreen(renderer, dev, helper, fontManager), true);
					return view;
				}

			public:
				ConcreteRunner(StartupScreenHelper *helper) : helper(helper) {}
				bool RunAndGetStartFlag() {
					this->Run(800, 480);
					return view->startRequested;
				}
			};

			bool startFlag;
			{
				ConcreteRunner runner(helper);
				runner.SetHasSystemMenu(true);
				startFlag = runner.RunAndGetStartFlag();
			}

			if (startFlag)
				spades::StartMainScreen();
		}
	}
}
