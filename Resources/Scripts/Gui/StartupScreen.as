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

#include "Flags.as"

namespace spades {
	

	class StartupScreenUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Font@ font;
		StartupScreenHelper@ helper;
		
		spades::ui::UIManager@ manager;
		
		StartupScreenMainMenu@ mainMenu;
		
		bool shouldExit = false;
		
		StartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, Font@ font, StartupScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.font = font;
			@this.helper = helper;
			
			SetupRenderer();
			
			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = font;
			
			@mainMenu = StartupScreenMainMenu(this);
			mainMenu.Bounds = manager.RootElement.Bounds;
			manager.RootElement.AddChild(mainMenu);
			
		}
		
		void SetupRenderer() {
			if(manager !is null)	
				manager.KeyPanic();
		}
		
		void MouseEvent(float x, float y) {
			manager.MouseEvent(x, y);
		}
		
		void WheelEvent(float x, float y) {
			manager.WheelEvent(x, y);
		}
		
		void KeyEvent(string key, bool down) {
			manager.KeyEvent(key, down);
		}
		
		void TextInputEvent(string text) {
			manager.TextInputEvent(text);
		}
		
		void TextEditingEvent(string text, int start, int len) {
			manager.TextEditingEvent(text, start, len);
		}
		
		bool AcceptsTextInput() {
			return manager.AcceptsTextInput;
		}
		
		AABB2 GetTextInputRect() {
			return manager.TextInputRect;
		}
		
		void RunFrame(float dt) {
			renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 1.f);
			renderer.DrawImage(renderer.RegisterImage("Gfx/White.tga"),
				AABB2(0.f, 0.f, renderer.ScreenWidth, renderer.ScreenHeight));
		
			
			// draw title logo
			Image@ img = renderer.RegisterImage("Gfx/Title/Logo.png");
			renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 1.f);
			renderer.DrawImage(img, Vector2((renderer.ScreenWidth - img.Width) * 0.5f, 64.f));
			
			manager.RunFrame(dt);
			manager.Render();
			
			renderer.FrameDone();
			renderer.Flip();
		}
		
		void Closing() {
			shouldExit = true;
		}
		
		bool WantsToBeClosed() {
			return shouldExit;
		}
	}
	
	
	
	class StartupScreenMainMenu: spades::ui::UIElement {
		
		StartupScreenUI@ ui;
		StartupScreenHelper@ helper;
		
		spades::ui::ListView@ serverList;
		
		private ConfigItem cg_protocolVersion("cg_protocolVersion");
		private ConfigItem cg_lastQuickConnectHost("cg_lastQuickConnectHost");
		private ConfigItem cg_serverlistSort("cg_serverlistSort", "16385");
		
		StartupScreenMainMenu(StartupScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			
			float width = Manager.Renderer.ScreenWidth;
			float height = Manager.Renderer.ScreenHeight;
			{
				spades::ui::Button button(Manager);
				button.Caption = "Start";
				button.Bounds = AABB2((width - 150.f) * 0.5f, height - 100.f, 150.f, 30.f);
				button.Activated = EventHandler(this.OnStartPressed);
				AddChild(button);
			}
			
		}
		
		
		private void OnQuitPressed(spades::ui::UIElement@ sender) {
			ui.shouldExit = true;
		}
		
		private void OnSetupPressed(spades::ui::UIElement@ sender) {
			PreferenceView al(this, PreferenceViewOptions());
			al.Run();
		}
		
		private void Start() {
			helper.Start();
			ui.shouldExit = true; // we have to exit from the startup screen to start the game
		}
		
		private void OnStartPressed(spades::ui::UIElement@ sender) {
			Start();
		}
		
		void HotKey(string key) {
			if(IsEnabled and key == "Enter") {
				Start();
			} else if(IsEnabled and key == "Escape") {
				ui.shouldExit = true;
			} else {
				UIElement::HotKey(key);
			}
		}
		
		void Render() {
			UIElement::Render();
			
		}
	}
	
	
	StartupScreenUI@ CreateStartupScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, StartupScreenHelper@ helper) {
		return StartupScreenUI(renderer, audioDevice, font, helper);
	}
}
