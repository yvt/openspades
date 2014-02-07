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
		Font@ font;
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
			Image@ img = renderer.RegisterImage("Gfx/Title/LogoSmall.png");
			renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 1.f);
			renderer.DrawImage(img, AABB2(10.f, 10.f, img.Width, img.Height));
			
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
		spades::ui::CheckBox@ bypassStartupWindowCheck;
		
		private ConfigItem cl_showStartupWindow("cl_showStartupWindow", "-1");
		
		StartupScreenMainMenu(StartupScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			
			@this.Font = ui.font;
			
			float width = Manager.Renderer.ScreenWidth;
			float height = Manager.Renderer.ScreenHeight;
			{
				spades::ui::Button button(Manager);
				button.Caption = "Start";
				button.Bounds = AABB2(width - 170.f, 20.f, 150.f, 30.f);
				button.Activated = EventHandler(this.OnStartPressed);
				AddChild(button);
			}
			{
				spades::ui::CheckBox button(Manager);
				button.Caption = "Skip this screen next time";
				button.Bounds = AABB2(360.f, 62.f, width - 380.f, 20.f);
				AddChild(button);
				@bypassStartupWindowCheck = button;
				button.Activated = EventHandler(this.OnBypassStartupWindowCheckChanged);
				switch(cl_showStartupWindow.IntValue) {
				case -1:
					button.Toggled = false;
					break;
				case 0:
					button.Toggled = true;
					break;
				default:
					button.Toggled = false;
					break;
				}
				
			}
			
			spades::ui::UIElement graphicsTab(Manager);
			spades::ui::UIElement audioTab(Manager);
			spades::ui::UIElement profileTab(Manager);
			AABB2 clientArea(10.f, 100.f, width - 20.f, height - 110.f);
			graphicsTab.Bounds = clientArea;
			audioTab.Bounds = clientArea;
			profileTab.Bounds = clientArea;
			AddChild(graphicsTab);
			AddChild(audioTab);
			AddChild(profileTab);
			audioTab.Visible = false;
			profileTab.Visible = false;
			
			
			
			{
				spades::ui::SimpleTabStrip tabStrip(Manager);
				AddChild(tabStrip);
				tabStrip.Bounds = AABB2(10.f, 70.f, width - 20.f, 24.f);
				tabStrip.AddItem("Graphics", graphicsTab);
				tabStrip.AddItem("Audio", audioTab);
				tabStrip.AddItem("System Info", profileTab);
				
			}
		}
		
		private void OnBypassStartupWindowCheckChanged(spades::ui::UIElement@ sender) {
			cl_showStartupWindow.IntValue = (bypassStartupWindowCheck.Toggled ? 0 : 1);
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
