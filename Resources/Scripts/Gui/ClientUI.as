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

	class CommandHistoryItem {
		string text;
		int selStart;
		int selEnd;
		CommandHistoryItem() {}
		CommandHistoryItem(string text, int selStart, int selEnd) {
			this.text = text;
			this.selStart = selStart;
			this.selEnd = selEnd;
		}
	}

	class ClientUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		FontManager@ fontManager;
		ClientUIHelper@ helper;

		spades::ui::UIManager@ manager;
		spades::ui::UIElement@ activeUI;

		ChatLogWindow@ chatLogWindow;

		ClientMenu@ clientMenu;

		array<spades::ui::CommandHistoryItem@> chatHistory;

		bool shouldExit = false;

		private float time = -1.f;

		ClientUI(Renderer@ renderer, AudioDevice@ audioDevice, FontManager@ fontManager, ClientUIHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.fontManager = fontManager;
			@this.helper = helper;

			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = fontManager.GuiFont;

			@clientMenu = ClientMenu(this);
			clientMenu.Bounds = manager.RootElement.Bounds;

			@chatLogWindow = ChatLogWindow(this);
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

		void RecordChatLog(string text, Vector4 color) {
			chatLogWindow.Record(text, color);
		}
	}

	class ClientMenu: spades::ui::UIElement {
		private ClientUI@ ui;
		private ClientUIHelper@ helper;

		ClientMenu(ClientUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;

			float winW = 180.f, winH = 32.f * 4.f - 2.f;
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
				button.Caption = _Tr("Client", "Back to Game");
				button.Bounds = AABB2(winX, winY, winW, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnBackToGame);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Chat Log");
				button.Bounds = AABB2(winX, winY + 32.f, winW, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnChatLog);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Setup");
				button.Bounds = AABB2(winX, winY + 64.f, winW, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnSetup);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Disconnect");
				button.Bounds = AABB2(winX, winY + 96.f, winW, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnDisconnect);
				AddChild(button);
			}
		}

		private void OnBackToGame(spades::ui::UIElement@ sender) {
			@ui.ActiveUI = null;
		}
		private void OnSetup(spades::ui::UIElement@ sender) {
			PreferenceViewOptions opt;
			opt.GameActive = true;

			PreferenceView al(this, opt, ui.fontManager);
			al.Run();
		}
		private void OnChatLog(spades::ui::UIElement@ sender) {
			@ui.ActiveUI = @ui.chatLogWindow;
			ui.chatLogWindow.ScrollToEnd();
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

	// Field with bash-like history support
	class FieldWithHistory: spades::ui::Field {
		array<spades::ui::CommandHistoryItem@>@ cmdhistory;
		CommandHistoryItem@ temporalLastHistory;
		uint currentHistoryIndex;

		FieldWithHistory(spades::ui::UIManager@ manager, array<spades::ui::CommandHistoryItem@>@ history) {
			super(manager);

			@this.cmdhistory = history;
			currentHistoryIndex = history.length;
			@temporalLastHistory = this.CommandHistoryItemRep;
		}

		private CommandHistoryItem@ CommandHistoryItemRep {
			get {
				return CommandHistoryItem(this.Text, this.SelectionStart, this.SelectionEnd);
			}
			set {
				this.Text = value.text;
				this.Select(value.selStart, value.selEnd - value.selStart);
			}
		}

		private void OverwriteItem() {
			if(currentHistoryIndex < cmdhistory.length) {
				@cmdhistory[currentHistoryIndex] = this.CommandHistoryItemRep;
			}else if(currentHistoryIndex == cmdhistory.length) {
				@temporalLastHistory = this.CommandHistoryItemRep;
			}
		}

		private void LoadItem() {
			if(currentHistoryIndex < cmdhistory.length) {
				@this.CommandHistoryItemRep = cmdhistory[currentHistoryIndex];
			}else if(currentHistoryIndex == cmdhistory.length) {
				@this.CommandHistoryItemRep = temporalLastHistory;
			}
		}

		void KeyDown(string key) {
			if(key == "Up") {
				if(currentHistoryIndex > 0) {
					OverwriteItem();
					currentHistoryIndex--;
					LoadItem();
				}
			}else if(key == "Down") {
				if(currentHistoryIndex < cmdhistory.length) {
					OverwriteItem();
					currentHistoryIndex++;
					LoadItem();
				}
			}else{
				Field::KeyDown(key);
			}
		}

		void CommandSent() {
			cmdhistory.insertLast(this.CommandHistoryItemRep);
			currentHistoryIndex = cmdhistory.length - 1;
		}

		void Cancelled() {
			OverwriteItem();
		}
	};

	uint StringCommonPrefixLength(string a, string b) {
		for(uint i = 0, ln = Min(a.length, b.length); i < ln; i++) {
			if(ToLower(a[i]) != ToLower(b[i])) return i;
		}
		return Min(a.length, b.length);
	}

	/** Shows cvar's current value when user types something like "/cg_foobar" */
	class CommandFieldConfigValueView: spades::ui::UIElement {
		string[]@ configNames;
		string[] configValues;
		CommandFieldConfigValueView(spades::ui::UIManager@ manager, string[] configNames) {
			super(manager);
			for(uint i = 0, len = configNames.length; i < len; i++) {
				configValues.insertLast(ConfigItem(configNames[i]).StringValue);
			}
			@this.configNames = configNames;
		}
		void Render() {
			float maxNameLen = 0.f;
			float maxValueLen = 20.f;
			Font@ font = this.Font;
			Renderer@ renderer = this.Manager.Renderer;
			float rowHeight = 25.f;

			for(uint i = 0, len = configNames.length; i < len; i++) {
				maxNameLen = Max(maxNameLen, font.Measure(configNames[i]).x);
				maxValueLen = Max(maxValueLen, font.Measure(configValues[i]).x);
			}
			Vector2 pos = this.ScreenPosition;
			pos.y -= float(configNames.length) * rowHeight + 10.f;

			renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.5f);
			renderer.DrawImage(null,
				AABB2(pos.x, pos.y, maxNameLen + maxValueLen + 20.f,
				      float(configNames.length) * rowHeight + 10.f));

			for(uint i = 0, len = configNames.length; i < len; i++) {
				font.DrawShadow(configNames[i],
					pos + Vector2(5.f, 8.f + float(i) * rowHeight),
					1.f, Vector4(1,1,1,0.7), Vector4(0,0,0,0.3f));
				font.DrawShadow(configValues[i],
					pos + Vector2(15.f + maxNameLen, 8.f + float(i) * rowHeight),
					1.f, Vector4(1,1,1,1), Vector4(0,0,0,0.4f));
			}

		}
	}

	class CommandField: FieldWithHistory {
		CommandFieldConfigValueView@ valueView;

		CommandField(spades::ui::UIManager@ manager, array<spades::ui::CommandHistoryItem@>@ history) {
			super(manager, history);

		}

		void OnChanged() {
			FieldWithHistory::OnChanged();

			if(valueView !is null) {
				@valueView.Parent = null;
			}
			if(Text.substr(0, 1) == "/" &&
			   Text.substr(1, 1) != " ") {
				int whitespace = Text.findFirst(" ");
				if(whitespace < 0) {
					whitespace = int(Text.length);
				}

				string input = Text.substr(1, whitespace - 1);
				if(input.length >= 2) {
					string[]@ names = GetAllConfigNames();
					string[] filteredNames;
					for(uint i = 0, len = names.length; i < len; i++) {
						if(StringCommonPrefixLength(input, names[i]) == input.length) {
							filteredNames.insertLast(names[i]);
							if(filteredNames.length >= 8) {
								// too many
								break;
							}
						}
					}
					if(filteredNames.length > 0) {
						@valueView = CommandFieldConfigValueView(this.Manager, filteredNames);
						valueView.Bounds = AABB2(0.f, -15.f, 0.f, 0.f);
						@valueView.Parent = this;
					}
				}
			}
		}

		void KeyDown(string key) {
			if(key == "Tab") {
				if(SelectionLength == 0 &&
				   SelectionStart == int(Text.length) &&
				   Text.substr(0, 1) == "/" &&
				   Text.findFirst(" ") < 0) {
					// config variable auto completion
					string input = Text.substr(1);
					string[]@ names = GetAllConfigNames();
					string commonPart;
					bool foundOne = false;
					for(uint i = 0, len = names.length; i < len; i++) {
						if(StringCommonPrefixLength(input, names[i]) == input.length) {
							if(!foundOne) {
								commonPart = names[i];
								foundOne = true;
							}

							uint commonLen = StringCommonPrefixLength(commonPart, names[i]);
							commonPart = commonPart.substr(0, commonLen);
						}
					}

					if(commonPart.length > input.length) {
						Text = "/" + commonPart;
						Select(Text.length, 0);
					}

				}
			}else{
				FieldWithHistory::KeyDown(key);
			}
		}
	}

	class ClientChatWindow: spades::ui::UIElement {
		private ClientUI@ ui;
		private ClientUIHelper@ helper;

		CommandField@ field;
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
				button.Caption = _Tr("Client", "Say");
				button.Bounds = AABB2(winX + winW - 244.f, winY + 36.f, 120.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnSay);
				AddChild(button);
				@sayButton = button;
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Cancel");
				button.Bounds = AABB2(winX + winW - 120.f, winY + 36.f, 120.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnCancel);
				AddChild(button);
			}
			{
				@field = CommandField(Manager, ui.chatHistory);
				field.Bounds = AABB2(winX, winY, winW, 30.f);
				field.Placeholder = _Tr("Client", "Chat Text");
				@field.Changed = spades::ui::EventHandler(this.OnFieldChanged);
				AddChild(field);
			}
			{
				@globalButton = spades::ui::SimpleButton(Manager);
				globalButton.Toggle = true;
				globalButton.Toggled = isTeamChat == false;
				globalButton.Caption = _Tr("Client", "Global");
				globalButton.Bounds = AABB2(winX, winY + 36.f, 70.f, 30.f);
				@globalButton.Activated = spades::ui::EventHandler(this.OnSetGlobal);
				AddChild(globalButton);
			}
			{
				@teamButton = spades::ui::SimpleButton(Manager);
				teamButton.Toggle = true;
				teamButton.Toggled = isTeamChat == true;
				teamButton.Caption = _Tr("Client", "Team");
				teamButton.Bounds = AABB2(winX + 70.f, winY + 36.f, 70.f, 30.f);
				@teamButton.Activated = spades::ui::EventHandler(this.OnSetTeam);
				AddChild(teamButton);
			}
		}

		void UpdateState() {
			sayButton.Enable = field.Text.length > 0;
		}

		bool IsTeamChat {
			get final { return isTeamChat; }
			set {
				if(isTeamChat == value) return;
				isTeamChat = value;
				teamButton.Toggled = isTeamChat;
				globalButton.Toggled = not isTeamChat;
				UpdateState();
			}
		}

		private void OnSetGlobal(spades::ui::UIElement@ sender) {
			IsTeamChat = false;
		}
		private void OnSetTeam(spades::ui::UIElement@ sender) {
			IsTeamChat = true;
		}

		private void OnFieldChanged(spades::ui::UIElement@ sender) {
			UpdateState();
		}

		private void Close() {
			@ui.ActiveUI = null;
		}

		private void OnCancel(spades::ui::UIElement@ sender) {
			field.Cancelled();
			Close();
		}

		private bool CheckConfigVariableSet() {
			string text = field.Text;
			if(text.substr(0, 1) != "/") return false;
			int idx = text.findFirst(" ");
			if(idx < 2) return false;

			// find variable
			string varname = text.substr(1, idx - 1);
			string[] vars = GetAllConfigNames();

			for(uint i = 0, len = vars.length; i < len; i++) {
				if(vars[i].length == varname.length &&
				   StringCommonPrefixLength(vars[i], varname) == vars[i].length) {
					// match
					string val = text.substr(idx + 1);
					ConfigItem item(vars[i]);
					item.StringValue = val;
				}
			}

			return false;
		}

		private void OnSay(spades::ui::UIElement@ sender) {
			field.CommandSent();
			if(!CheckConfigVariableSet()) {
				if(isTeamChat)
					ui.helper.SayTeam(field.Text);
				else
					ui.helper.SayGlobal(field.Text);
			}
			Close();
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

	class ChatLogSayWindow: ClientChatWindow {
		ChatLogWindow@ owner;

		ChatLogSayWindow(ChatLogWindow@ own, bool isTeamChat) {
			super(own.ui, isTeamChat);
			@owner = own;
		}

		void Close() {
			owner.SayWindowClosed();
			@this.Parent = null;
		}
	}

	class ChatLogWindow: spades::ui::UIElement {

		float contentsTop, contentsHeight;

		ClientUI@ ui;
		private ClientUIHelper@ helper;

		private spades::ui::TextViewer@ viewer;
		ChatLogSayWindow@ sayWindow;

		private spades::ui::UIElement@ sayButton1;
		private spades::ui::UIElement@ sayButton2;

		ChatLogWindow(ClientUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;

			@Font = Manager.RootElement.Font;
			this.Bounds = Manager.RootElement.Bounds;

			float contentsWidth = 700.f;
			float contentsLeft = (Manager.Renderer.ScreenWidth - contentsWidth) * 0.5f;
			contentsHeight = Manager.Renderer.ScreenHeight - 200.f;
			contentsTop = (Manager.Renderer.ScreenHeight - contentsHeight - 106.f) * 0.5f;
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.4f);
				label.Bounds = Bounds;
				AddChild(label);
			}
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.8f);
				label.Bounds = AABB2(0.f, contentsTop - 13.f, Size.x, contentsHeight + 27.f);
				AddChild(label);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Close");
				button.Bounds = AABB2(
					contentsLeft + contentsWidth - 150.f,
					contentsTop + contentsHeight - 30.f
					, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnOkPressed);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Say Global");
				button.Bounds = AABB2(
					contentsLeft,
					contentsTop + contentsHeight - 30.f
					, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnGlobalChat);
				AddChild(button);
				@this.sayButton1 = button;
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("Client", "Say Team");
				button.Bounds = AABB2(
					contentsLeft + 155.f,
					contentsTop + contentsHeight - 30.f
					, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnTeamChat);
				AddChild(button);
				@this.sayButton2 = button;
			}
			{
				spades::ui::TextViewer viewer(Manager);
				AddChild(viewer);
				viewer.Bounds = AABB2(contentsLeft, contentsTop, contentsWidth, contentsHeight - 40.f);
				@this.viewer = viewer;
			}
		}

		void ScrollToEnd() {
			viewer.Layout();
			viewer.ScrollToEnd();
		}

		void Close() {
			@ui.ActiveUI = null;
		}

		void SayWindowClosed() {
			@sayWindow = null;
			sayButton1.Enable = true;
			sayButton2.Enable = true;
		}

		private void OnOkPressed(spades::ui::UIElement@ sender) {
			Close();
		}

		private void OnTeamChat(spades::ui::UIElement@ sender) {
			if(sayWindow !is null) {
				sayWindow.IsTeamChat = true;
				return;
			}
			sayButton1.Enable = false;
			sayButton2.Enable = false;
			ChatLogSayWindow wnd(this, true);
			AddChild(wnd);
			wnd.Bounds = this.Bounds;
			@this.sayWindow = wnd;
			@Manager.ActiveElement = wnd.field;
		}

		private void OnGlobalChat(spades::ui::UIElement@ sender) {
			if(sayWindow !is null) {
				sayWindow.IsTeamChat = false;
				return;
			}
			sayButton1.Enable = false;
			sayButton2.Enable = false;
			ChatLogSayWindow wnd(this, false);
			AddChild(wnd);
			wnd.Bounds = this.Bounds;
			@this.sayWindow = wnd;
			@Manager.ActiveElement = wnd.field;
		}

		void HotKey(string key) {
			if(sayWindow !is null) {
				UIElement::HotKey(key);
				return;
			}
			if(IsEnabled and (key == "Escape")) {
				Close();
			} else if(IsEnabled and (key == "y")) {
				OnTeamChat(this);
			} else if(IsEnabled and (key == "t")) {
				OnGlobalChat(this);
			} else {
				UIElement::HotKey(key);
			}
		}

		void Record(string text, Vector4 color) {
			viewer.AddLine(text, this.IsVisible, color);
		}

		void Render() {
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Renderer@ r = Manager.Renderer;
			Image@ img = r.RegisterImage("Gfx/White.tga");

			r.ColorNP = Vector4(1, 1, 1, 0.08f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop - 15.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop + contentsHeight + 15.f, size.x, 1.f));
			r.ColorNP = Vector4(1, 1, 1, 0.2f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop - 14.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop + contentsHeight + 14.f, size.x, 1.f));

			UIElement::Render();
		}

	}

	ClientUI@ CreateClientUI(Renderer@ renderer, AudioDevice@ audioDevice,
		FontManager@ fontManager, ClientUIHelper@ helper) {
		return ClientUI(renderer, audioDevice, fontManager, helper);
	}
}
