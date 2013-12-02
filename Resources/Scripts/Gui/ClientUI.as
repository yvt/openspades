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
				manager.RootElement.RemoveChild(activeUI);
			}
			@activeUI = value;
			if(activeUI !is null) {
				activeUI.Bounds = manager.RootElement.Bounds;
				manager.RootElement.AddChild(activeUI);
			}
			manager.KeyPanic();
		}
		spades::ui::UIElement@ get_ActiveUI(){ 
			return activeUI; 
		}
		
		void EnterClientMenu() {
			@ActiveUI = clientMenu;
		}
		
		void EnterTeamChatWindow() {
			ClientChatWindow wnd(this, true);
			@ActiveUI = wnd;
			@manager.ActiveElement = wnd.field;
		}
		void EnterGlobalChatWindow() {
			ClientChatWindow wnd(this, false);
			@ActiveUI = wnd;
			@manager.ActiveElement = wnd.field;
		}
		void EnterCommandWindow() {
			ClientChatWindow wnd(this, true);
			wnd.field.Text = "/";
			wnd.field.Select(1, 0);
			wnd.UpdateState();
			@ActiveUI = wnd;
			@manager.ActiveElement = wnd.field;
		}
		void CloseUI() {
			@ActiveUI = null;
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
				label.Bounds = AABB2(0.f, 0.f, 
					Manager.Renderer.ScreenWidth, Manager.Renderer.ScreenHeight);
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
	
	class ClientChatWindow: spades::ui::UIElement {
		private ClientUI@ ui;
		private ClientUIHelper@ helper;
		
		spades::ui::Field@ field;
		spades::ui::Button@ sayButton;
		spades::ui::SimpleButton@ teamButton;
		spades::ui::SimpleButton@ globalButton;
		
		bool isTeamChat;
		
		ClientChatWindow(ClientUI@ ui, bool isTeamChat) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			this.isTeamChat = isTeamChat;
			
			float winW = Manager.Renderer.ScreenWidth * 0.7f, winH = 66.f;
			float winX = (Manager.Renderer.ScreenWidth - winW) * 0.5f;
			float winY = (Manager.Renderer.ScreenHeight - winH) - 20.f;
			/*
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
				label.Bounds = Bounds;
				AddChild(label);
			}*/
			
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
				label.Bounds = AABB2(winX - 8.f, winY - 8.f, winW + 16.f, winH + 16.f);
				AddChild(label);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = "Say";
				button.Bounds = AABB2(winX + winW - 244.f, winY + 36.f, 120.f, 30.f);
				button.Activated = EventHandler(this.OnSay);
				AddChild(button);
				@sayButton = button;
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = "Cancel";
				button.Bounds = AABB2(winX + winW - 120.f, winY + 36.f, 120.f, 30.f);
				button.Activated = EventHandler(this.OnCancel);
				AddChild(button);
			}
			{
				@field = spades::ui::Field(Manager);
				field.Bounds = AABB2(winX, winY, winW, 30.f);
				field.Placeholder = "Chat Text";
				field.Changed = spades::ui::EventHandler(this.OnFieldChanged);
				AddChild(field);
			}
			{
				@globalButton = spades::ui::SimpleButton(Manager);
				globalButton.Toggle = true;
				globalButton.Toggled = isTeamChat == false;
				globalButton.Caption = "Global";
				globalButton.Bounds = AABB2(winX, winY + 36.f, 70.f, 30.f);
				globalButton.Activated = EventHandler(this.OnSetGlobal);
				AddChild(globalButton);
			}
			{
				@teamButton = spades::ui::SimpleButton(Manager);
				teamButton.Toggle = true;
				teamButton.Toggled = isTeamChat == true;
				teamButton.Caption = "Team";
				teamButton.Bounds = AABB2(winX + 70.f, winY + 36.f, 70.f, 30.f);
				teamButton.Activated = EventHandler(this.OnSetTeam);
				AddChild(teamButton);
			}
		}
		
		void UpdateState() {
			sayButton.Enable = field.Text.length > 0;
		}
		
		private void OnSetGlobal(spades::ui::UIElement@ sender) {
			teamButton.Toggled = false;
			globalButton.Toggled = true;
			isTeamChat = false;
			UpdateState();
		}
		private void OnSetTeam(spades::ui::UIElement@ sender) {
			teamButton.Toggled = true;
			globalButton.Toggled = false;
			isTeamChat = true;
			UpdateState();
		}
		
		private void OnFieldChanged(spades::ui::UIElement@ sender) {
			UpdateState();
		}
		
		private void OnCancel(spades::ui::UIElement@ sender) {
			@ui.ActiveUI = null;
		}
		private void OnSay(spades::ui::UIElement@ sender) {
			if(isTeamChat)
				ui.helper.SayTeam(field.Text);
			else
				ui.helper.SayGlobal(field.Text);
			@ui.ActiveUI = null;
		}
		
		void HotKey(string key) {
			if(IsEnabled and key == "Escape") {
				OnCancel(this);
			}else if(IsEnabled and key == "Enter") {
				if(field.Text.length == 0) {
					OnCancel(this);
				}else{
					OnSay(this);
				}
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
