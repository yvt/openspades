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


namespace spades {
	
	class ClientUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Font@ font;
		ClientUIHelper@ helper;
		
		spades::ui::UIManager@ manager;
		spades::ui::UIElement@ activeUI;
		
		ClientMenu@ clientMenu;
		
		bool shouldExit = false;
		
		private float time = -1.f;
		
		ClientUI(Renderer@ renderer, AudioDevice@ audioDevice, Font@ font, ClientUIHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.font = font;
			@this.helper = helper;
			
			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = font;
			
			@clientMenu = ClientMenu(this);
			clientMenu.Bounds = manager.RootElement.Bounds;
			clientMenu.Visible = false;
			manager.RootElement.AddChild(clientMenu);
			
		}
		
		void MouseEvent(float x, float y) {
			manager.MouseEvent(x, y);
		}
		
		void KeyEvent(string key, bool down) {
			manager.KeyEvent(key, down);
		}
		
		void CharEvent(string text) {
			manager.CharEvent(text);
		}
		
		void RunFrame(float dt) {
			if(time < 0.f) {
				time = 0.f;
			}
			
			manager.RunFrame(dt);
			if(activeUI !is null){
				manager.Render();
			}
			
			time += Min(dt, 0.05f);
		}
		
		void Closing() {
			
		}
		
		bool WantsClientToBeClosed() {
			return shouldExit;
		}
		
		bool NeedsInput() {
			return activeUI !is null;
		}
		
		void set_ActiveUI(spades::ui::UIElement@ value) {
			if(activeUI !is null) {
				activeUI.Visible = false;
			}
			@activeUI = value;
			if(activeUI !is null) {
				activeUI.Visible = true;
			}
		}
		spades::ui::UIElement@ get_ActiveUI(){ 
			return activeUI; 
		}
		
		void EnterClientMenu() {
			@ActiveUI = clientMenu;
		}
	}
	
	class ClientMenu: spades::ui::UIElement {
		private ClientUI@ ui;
		private ClientUIHelper@ helper;
		
		ClientMenu(ClientUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			
			float winW = 180.f, winH = 94.f;
			float winX = (Manager.Renderer.ScreenWidth - winW) * 0.5f;
			float winY = (Manager.Renderer.ScreenHeight - winH) * 0.5f;
			
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
				label.Bounds = Bounds;
				AddChild(label);
			}
			
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
				label.Bounds = AABB2(winX - 8.f, winY - 8.f, winW + 16.f, winH + 16.f);
				AddChild(label);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = "Back to Game";
				button.Bounds = AABB2(winX, winY, winW, 30.f);
				button.Activated = EventHandler(this.OnBackToGame);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = "Setup";
				button.Bounds = AABB2(winX, winY + 32.f, winW, 30.f);
				button.Activated = EventHandler(this.OnSetup);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = "Disconnect";
				button.Bounds = AABB2(winX, winY + 64.f, winW, 30.f);
				button.Activated = EventHandler(this.OnDisconnect);
				AddChild(button);
			}
		}
		
		private void OnBackToGame(spades::ui::UIElement@ sender) {
			@ui.ActiveUI = null;
		}
		private void OnSetup(spades::ui::UIElement@ sender) {
			PreferenceViewOptions opt;
			opt.GameActive = true;
			
			PreferenceView al(this, opt);
			al.Run();
		}
		private void OnDisconnect(spades::ui::UIElement@ sender) {
			ui.shouldExit = true;
		}
		
		void HotKey(string key) {
			if(IsEnabled and key == "Escape") {
				@ui.ActiveUI = null;
			} else {
				UIElement::HotKey(key);
			}
		}
	}
	
	ClientUI@ CreateClientUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, ClientUIHelper@ helper) {
		return ClientUI(renderer, audioDevice, font, helper);
	}
}
