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
#include "CreateProfileScreen.as"

namespace spades {


	class MainScreenUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		FontManager@ fontManager;
		MainScreenHelper@ helper;

		spades::ui::UIManager@ manager;

		MainScreenMainMenu@ mainMenu;

		bool shouldExit = false;

		private float time = -1.f;

        private ConfigItem cg_playerName("cg_playerName");
        private ConfigItem cg_playerNameIsSet("cg_playerNameIsSet", "0");

		MainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, FontManager@ fontManager, MainScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.fontManager = fontManager;
			@this.helper = helper;

			SetupRenderer();

			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = fontManager.GuiFont;

			@mainMenu = MainScreenMainMenu(this);
			mainMenu.Bounds = manager.RootElement.Bounds;
			manager.RootElement.AddChild(mainMenu);

			// Let the new player choose their IGN
			if (cg_playerName.StringValue != "" &&
				cg_playerName.StringValue != "Deuce") {
				cg_playerNameIsSet.IntValue = 1;
			}
			if (cg_playerNameIsSet.IntValue == 0) {
				CreateProfileScreen al(mainMenu);
				al.Run();
			}

		}

		void SetupRenderer() {
			// load map
			@renderer.GameMap = GameMap("Maps/Title.vxl");
			renderer.FogColor = Vector3(0.1f, 0.10f, 0.1f);
			renderer.FogDistance = 128.f;
			time = -1.f;

			// returned from the client game, so reload the server list.
			if(mainMenu !is null)
				mainMenu.LoadServerList();

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

		private SceneDefinition SetupCamera(SceneDefinition sceneDef,
			Vector3 eye, Vector3 at, Vector3 up, float fov) {
			Vector3 dir = (at - eye).Normalized;
			Vector3 side = Cross(dir, up).Normalized;
			up = -Cross(dir, side);
			sceneDef.viewOrigin = eye;
			sceneDef.viewAxisX = side;
			sceneDef.viewAxisY = up;
			sceneDef.viewAxisZ = dir;
			sceneDef.fovY = fov * 3.141592654f / 180.f;
			sceneDef.fovX = atan(tan(sceneDef.fovY * 0.5f) * renderer.ScreenWidth / renderer.ScreenHeight) * 2.f;
			return sceneDef;
		}

		void RunFrame(float dt) {
			if(time < 0.f) {
				time = 0.f;
			}

			SceneDefinition sceneDef;
			float cameraX = time;
			cameraX -= floor(cameraX / 512.f) * 512.f;
			cameraX = 512.f - cameraX;
			sceneDef = SetupCamera(sceneDef,
				Vector3(cameraX, 256.f, 12.f), Vector3(cameraX + .1f, 257.f, 12.5f), Vector3(0.f, 0.f, -1.f),
				30.f);
			sceneDef.zNear = 0.1f;
			sceneDef.zFar = 222.f;
			sceneDef.time = int(time * 1000.f);
			sceneDef.viewportWidth = int(renderer.ScreenWidth);
			sceneDef.viewportHeight = int(renderer.ScreenHeight);
			sceneDef.denyCameraBlur = true;
			sceneDef.depthOfFieldFocalLength = 100.f;
			sceneDef.skipWorld = false;

			// fade the map
			float fade = Clamp((time - 1.f) / 2.2f, 0.f, 1.f);
			sceneDef.globalBlur = Clamp((1.f - (time - 1.f) / 2.5f), 0.f, 1.f);
			if(!mainMenu.IsEnabled) {
				sceneDef.globalBlur = Max(sceneDef.globalBlur, 0.5f);
			}

			renderer.StartScene(sceneDef);
			renderer.EndScene();

			// fade the map (draw)
			if(fade < 1.f) {
				renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 1.f - fade);
				renderer.DrawImage(renderer.RegisterImage("Gfx/White.tga"),
					AABB2(0.f, 0.f, renderer.ScreenWidth, renderer.ScreenHeight));
			}

			// draw title logo
			Image@ img = renderer.RegisterImage("Gfx/Title/Logo.png");
			renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 1.f);
			renderer.DrawImage(img, Vector2((renderer.ScreenWidth - img.Width) * 0.5f, 64.f));

			manager.RunFrame(dt);
			manager.Render();

			renderer.FrameDone();
			renderer.Flip();
			time += Min(dt, 0.05f);
		}

		void Closing() {
			shouldExit = true;
		}

		bool WantsToBeClosed() {
			return shouldExit;
		}
	}

	class ServerListItem: spades::ui::ButtonBase {
		MainScreenServerItem@ item;
		FlagIconRenderer@ flagIconRenderer;
		ServerListItem(spades::ui::UIManager@ manager, MainScreenServerItem@ item){
			super(manager);
			@this.item = item;
			@flagIconRenderer = FlagIconRenderer(manager.Renderer);
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Image@ img = renderer.RegisterImage("Gfx/White.tga");

			Vector4 bgcolor = Vector4(1.f, 1.f, 1.f, 0.0f);
			Vector4 fgcolor = Vector4(1.f, 1.f, 1.f, 1.f);
			if(item.Favorite) {
				bgcolor = Vector4(0.3f, 0.3f, 1.f, 0.1f);
				fgcolor = Vector4(220.f/255.f,220.f/255.f,0,1);
			}
			if(Pressed && Hover) {
				bgcolor.w += 0.3;
			} else if(Hover) {
				bgcolor.w += 0.15;
			}
			renderer.ColorNP = bgcolor;
			renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));

			Font.Draw(item.Name, ScreenPosition + Vector2(4.f, 2.f), 1.f, fgcolor);
			string playersStr = ToString(item.NumPlayers) + "/" + ToString(item.MaxPlayers);
			Vector4 col(1,1,1,1);
			if(item.NumPlayers >= item.MaxPlayers) col = Vector4(1,0.7f,0.7f,1);
			else if(item.NumPlayers >= item.MaxPlayers * 3 / 4) col = Vector4(1,1,0.7f,1);
			else if(item.NumPlayers == 0) col = Vector4(0.7f,0.7f,1,1);
			Font.Draw(playersStr, ScreenPosition + Vector2(340.f-Font.Measure(playersStr).x * 0.5f, 2.f), 1.f, col);
			Font.Draw(item.MapName, ScreenPosition + Vector2(400.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.GameMode, ScreenPosition + Vector2(550.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.Protocol, ScreenPosition + Vector2(630.f, 2.f), 1.f, Vector4(1,1,1,1));
			if(not flagIconRenderer.DrawIcon(item.Country, ScreenPosition + Vector2(700.f, size.y * 0.5f))) {
				Font.Draw(item.Country, ScreenPosition + Vector2(680.f, 2.f), 1.f, Vector4(1,1,1,1));
			}
		}
	}

	funcdef void ServerListItemEventHandler(ServerListModel@ sender, MainScreenServerItem@ item);

	class ServerListModel: spades::ui::ListViewModel {
		spades::ui::UIManager@ manager;
		MainScreenServerItem@[]@ list;

		ServerListItemEventHandler@ ItemActivated;
		ServerListItemEventHandler@ ItemDoubleClicked;
		ServerListItemEventHandler@ ItemRightClicked;

		ServerListModel(spades::ui::UIManager@ manager, MainScreenServerItem@[]@ list) {
			@this.manager = manager;
			@this.list = list;
		}
		int NumRows {
			get { return int(list.length); }
		}
		private void OnItemClicked(spades::ui::UIElement@ sender){
			ServerListItem@ item = cast<ServerListItem>(sender);
			if(ItemActivated !is null) {
				ItemActivated(this, item.item);
			}
		}
		private void OnItemDoubleClicked(spades::ui::UIElement@ sender){
			ServerListItem@ item = cast<ServerListItem>(sender);
			if(ItemDoubleClicked !is null) {
				ItemDoubleClicked(this, item.item);
			}
		}
		private void OnItemRightClicked(spades::ui::UIElement@ sender){
			ServerListItem@ item = cast<ServerListItem>(sender);
			if(ItemRightClicked !is null) {
				ItemRightClicked(this, item.item);
			}
		}
		spades::ui::UIElement@ CreateElement(int row) {
			ServerListItem i(manager, list[row]);
			@i.Activated = spades::ui::EventHandler(this.OnItemClicked);
			@i.DoubleClicked = spades::ui::EventHandler(this.OnItemDoubleClicked);
			@i.RightClicked = spades::ui::EventHandler(this.OnItemRightClicked);
			return i;
		}
		void RecycleElement(spades::ui::UIElement@ elem) {}
	}


	class ServerListHeader: spades::ui::ButtonBase {
		string Text;
		ServerListHeader(spades::ui::UIManager@ manager){
			super(manager);
		}
		void OnActivated() {
			ButtonBase::OnActivated();
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Image@ img = renderer.RegisterImage("Gfx/White.tga");
			if(Pressed && Hover) {
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.3f);
			} else if(Hover) {
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.15f);
			} else {
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.0f);
			}
			renderer.DrawImage(img, AABB2(pos.x - 2.f, pos.y, size.x, size.y));

			Font.Draw(Text, ScreenPosition + Vector2(0.f, 2.f), 1.f, Vector4(1,1,1,1));
		}
	}


	class RefreshButton: spades::ui::SimpleButton {
		RefreshButton(spades::ui::UIManager@ manager){
			super(manager);
		}
		void Render() {
			SimpleButton::Render();

			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Image@ img = renderer.RegisterImage("Gfx/UI/Refresh.png");
			renderer.DrawImage(img, pos + (size - Vector2(16.f, 16.f)) * 0.5f);
		}
	}

	class ProtocolButton: spades::ui::SimpleButton {
		ProtocolButton(spades::ui::UIManager@ manager){
			super(manager);
			Toggle = true;
		}
	}

	uint8 ToLower(uint8 c) {
		if(c >= uint8(0x41) and c <= uint8(0x5a)) {
			return uint8(c - 0x41 + 0x61);
		} else {
			return c;
		}
	}
	bool StringContainsCaseInsensitive(string text, string pattern) {
		for(int i = text.length - 1; i >= 0; i--)
			text[i] = ToLower(text[i]);
		for(int i = pattern.length - 1; i >= 0; i--)
			pattern[i] = ToLower(pattern[i]);
		return text.findFirst(pattern) >= 0;
	}

	class MainScreenMainMenu: spades::ui::UIElement {

		MainScreenUI@ ui;
		MainScreenHelper@ helper;
		spades::ui::Field@ addressField;

		spades::ui::Button@ protocol3Button;
		spades::ui::Button@ protocol4Button;

		spades::ui::Button@ filterProtocol3Button;
		spades::ui::Button@ filterProtocol4Button;
		spades::ui::Button@ filterEmptyButton;
		spades::ui::Button@ filterFullButton;
		spades::ui::Field@ filterField;

		spades::ui::ListView@ serverList;
		MainScreenServerListLoadingView@ loadingView;
		MainScreenServerListErrorView@ errorView;
		bool loading = false, loaded = false;

		private ConfigItem cg_protocolVersion("cg_protocolVersion", "3");
		private ConfigItem cg_lastQuickConnectHost("cg_lastQuickConnectHost");
		private ConfigItem cg_serverlistSort("cg_serverlistSort", "16385");

		MainScreenMainMenu(MainScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;

			float contentsWidth = 750.f;
			float contentsLeft = (Manager.Renderer.ScreenWidth - contentsWidth) * 0.5f;
			float footerPos = Manager.Renderer.ScreenHeight - 50.f;
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("MainScreen", "Connect");
				button.Bounds = AABB2(contentsLeft + contentsWidth - 150.f, 200.f, 150.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnConnectPressed);
				AddChild(button);
			}
			{
				@addressField = spades::ui::Field(Manager);
				addressField.Bounds = AABB2(contentsLeft, 200, contentsWidth - 240.f, 30.f);
				addressField.Placeholder = _Tr("MainScreen", "Quick Connect");
				addressField.Text = cg_lastQuickConnectHost.StringValue;
				@addressField.Changed = spades::ui::EventHandler(this.OnAddressChanged);
				AddChild(addressField);
			}
			{
				@protocol3Button = ProtocolButton(Manager);
				protocol3Button.Bounds = AABB2(contentsLeft + contentsWidth - 240.f + 6.f, 200,
					40.f, 30.f);
				protocol3Button.Caption = _Tr("MainScreen", "0.75");
				@protocol3Button.Activated = spades::ui::EventHandler(this.OnProtocol3Pressed);
				protocol3Button.Toggle = true;
				protocol3Button.Toggled = cg_protocolVersion.IntValue == 3;
				AddChild(protocol3Button);
			}
			{
				@protocol4Button = ProtocolButton(Manager);
				protocol4Button.Bounds = AABB2(contentsLeft + contentsWidth - 200.f + 6.f, 200,
					40.f, 30.f);
				protocol4Button.Caption = _Tr("MainScreen", "0.76");
				@protocol4Button.Activated = spades::ui::EventHandler(this.OnProtocol4Pressed);
				protocol4Button.Toggle = true;
				protocol4Button.Toggled = cg_protocolVersion.IntValue == 4;
				AddChild(protocol4Button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("MainScreen", "Quit");
				button.Bounds = AABB2(contentsLeft + contentsWidth - 100.f, footerPos, 100.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnQuitPressed);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("MainScreen", "Credits");
				button.Bounds = AABB2(contentsLeft + contentsWidth - 202.f, footerPos, 100.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnCreditsPressed);
				AddChild(button);
			}
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("MainScreen", "Setup");
				button.Bounds = AABB2(contentsLeft + contentsWidth - 304.f, footerPos, 100.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnSetupPressed);
				AddChild(button);
			}
			{
				RefreshButton button(Manager);
				button.Bounds = AABB2(contentsLeft + contentsWidth - 364.f, footerPos, 30.f, 30.f);
				@button.Activated = spades::ui::EventHandler(this.OnRefreshServerListPressed);
				AddChild(button);
			}
			{
				spades::ui::Label label(Manager);
				label.Text = _Tr("MainScreen", "Filter");
				label.Bounds = AABB2(contentsLeft, footerPos, 50.f, 30.f);
				label.Alignment = Vector2(0.f, 0.5f);
				AddChild(label);
			}
			{
				@filterProtocol3Button = ProtocolButton(Manager);
				filterProtocol3Button.Bounds = AABB2(contentsLeft + 50.f, footerPos,
					40.f, 30.f);
				filterProtocol3Button.Caption = _Tr("MainScreen", "0.75");
				@filterProtocol3Button.Activated = spades::ui::EventHandler(this.OnFilterProtocol3Pressed);
				filterProtocol3Button.Toggle = true;
				AddChild(filterProtocol3Button);
			}
			{
				@filterProtocol4Button = ProtocolButton(Manager);
				filterProtocol4Button.Bounds = AABB2(contentsLeft + 90.f, footerPos,
					40.f, 30.f);
				filterProtocol4Button.Caption = _Tr("MainScreen", "0.76");
				@filterProtocol4Button.Activated = spades::ui::EventHandler(this.OnFilterProtocol4Pressed);
				filterProtocol4Button.Toggle = true;
				AddChild(filterProtocol4Button);
			}
			{
				@filterEmptyButton = ProtocolButton(Manager);
				filterEmptyButton.Bounds = AABB2(contentsLeft + 135.f, footerPos,
					50.f, 30.f);
				filterEmptyButton.Caption = _Tr("MainScreen", "Empty");
				@filterEmptyButton.Activated = spades::ui::EventHandler(this.OnFilterEmptyPressed);
				filterEmptyButton.Toggle = true;
				AddChild(filterEmptyButton);
			}
			{
				@filterFullButton = ProtocolButton(Manager);
				filterFullButton.Bounds = AABB2(contentsLeft + 185.f, footerPos,
					70.f, 30.f);
				filterFullButton.Caption = _Tr("MainScreen", "Not Full");
				@filterFullButton.Activated = spades::ui::EventHandler(this.OnFilterFullPressed);
				filterFullButton.Toggle = true;
				AddChild(filterFullButton);
			}
			{
				@filterField = spades::ui::Field(Manager);
				filterField.Bounds = AABB2(contentsLeft + 260.f, footerPos, 120.f, 30.f);
				filterField.Placeholder = _Tr("MainScreen", "Filter");
				@filterField.Changed = spades::ui::EventHandler(this.OnFilterTextChanged);
				AddChild(filterField);
			}
			{
				@serverList = spades::ui::ListView(Manager);
				serverList.Bounds = AABB2(contentsLeft, 270.f, contentsWidth, footerPos - 280.f);
				AddChild(serverList);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 2.f, 240.f, 300.f - 2.f, 30.f);
				header.Text = _Tr("MainScreen", "Server Name");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByName);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 300.f, 240.f, 100.f, 30.f);
				header.Text = _Tr("MainScreen", "Players");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByNumPlayers);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 400.f, 240.f, 150.f, 30.f);
				header.Text = _Tr("MainScreen", "Map Name");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByMapName);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 550.f, 240.f, 80.f, 30.f);
				header.Text = _Tr("MainScreen", "Game Mode");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByGameMode);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 630.f, 240.f, 50.f, 30.f);
				header.Text = _Tr("MainScreen", "Ver.");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByProtocol);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 680.f, 240.f, 50.f, 30.f);
				header.Text = _Tr("MainScreen", "Loc.");
				@header.Activated = spades::ui::EventHandler(this.SortServerListByCountry);
				AddChild(header);
			}
			{
				@loadingView = MainScreenServerListLoadingView(Manager);
				loadingView.Bounds = AABB2(contentsLeft, 240.f, contentsWidth, 100.f);
				loadingView.Visible = false;
				AddChild(loadingView);
			}
			{
				@errorView = MainScreenServerListErrorView(Manager);
				errorView.Bounds = AABB2(contentsLeft, 240.f, contentsWidth, 100.f);
				errorView.Visible = false;
				AddChild(errorView);
			}
			LoadServerList();
		}

		void LoadServerList() {
			if(loading) {
				return;
			}
			loaded = false;
			loading = true;
			@serverList.Model = spades::ui::ListViewModel(); // empty
			errorView.Visible = false;
			loadingView.Visible = true;
			helper.StartQuery();
		}

		void ServerListItemActivated(ServerListModel@ sender, MainScreenServerItem@ item) {
			addressField.Text = item.Address;
			cg_lastQuickConnectHost = addressField.Text;
			if(item.Protocol == "0.75") {
				SetProtocolVersion(3);
			}else if(item.Protocol == "0.76") {
				SetProtocolVersion(4);
			}
			addressField.SelectAll();
		}

		void ServerListItemDoubleClicked(ServerListModel@ sender, MainScreenServerItem@ item) {
			ServerListItemActivated(sender, item);

			// Double-click to connect
			Connect();
		}

		void ServerListItemRightClicked(ServerListModel@ sender, MainScreenServerItem@ item) {
			helper.SetServerFavorite(item.Address, !item.Favorite);
			UpdateServerList();
		}

		private void SortServerListByPing(spades::ui::UIElement@ sender) {
			SortServerList(0);
		}
		private void SortServerListByNumPlayers(spades::ui::UIElement@ sender) {
			SortServerList(1);
		}
		private void SortServerListByName(spades::ui::UIElement@ sender) {
			SortServerList(2);
		}
		private void SortServerListByMapName(spades::ui::UIElement@ sender) {
			SortServerList(3);
		}
		private void SortServerListByGameMode(spades::ui::UIElement@ sender) {
			SortServerList(4);
		}
		private void SortServerListByProtocol(spades::ui::UIElement@ sender) {
			SortServerList(5);
		}
		private void SortServerListByCountry(spades::ui::UIElement@ sender) {
			SortServerList(6);
		}

		private void SortServerList(int keyId) {
			int sort = cg_serverlistSort.IntValue;
			if(int(sort & 0xfff) == keyId) {
				sort ^= int(0x4000);
			} else {
				sort = keyId;
			}
			cg_serverlistSort = sort;
			UpdateServerList();
		}

		private void UpdateServerList() {
			string key = "";
			switch(cg_serverlistSort.IntValue & 0xfff) {
				case 0: key = "Ping"; break;
				case 1: key = "NumPlayers"; break;
				case 2: key = "Name"; break;
				case 3: key = "MapName"; break;
				case 4: key = "GameMode"; break;
				case 5: key = "Protocol"; break;
				case 6: key = "Country"; break;
			}
			MainScreenServerItem@[]@ list = helper.GetServerList(key,
				(cg_serverlistSort.IntValue & 0x4000) != 0);
			if((list is null) or (loading)){
				@serverList.Model = spades::ui::ListViewModel(); // empty
				return;
			}

			// filter the server list
			bool filterProtocol3 = filterProtocol3Button.Toggled;
			bool filterProtocol4 = filterProtocol4Button.Toggled;
			bool filterEmpty = filterEmptyButton.Toggled;
			bool filterFull = filterFullButton.Toggled;
			string filterText = filterField.Text;
			MainScreenServerItem@[]@ list2 = array<spades::MainScreenServerItem@>();
			for(int i = 0, count = list.length; i < count; i++) {
				MainScreenServerItem@ item = list[i];
				if(filterProtocol3 and (item.Protocol != "0.75")) {
					continue;
				}
				if(filterProtocol4 and (item.Protocol != "0.76")) {
					continue;
				}
				if(filterEmpty and (item.NumPlayers > 0)) {
					continue;
				}
				if(filterFull and (item.NumPlayers >= item.MaxPlayers)) {
					continue;
				}
				if(filterText.length > 0) {
					if(not (StringContainsCaseInsensitive(item.Name, filterText) or
						StringContainsCaseInsensitive(item.MapName, filterText) or
						StringContainsCaseInsensitive(item.GameMode, filterText))) {
						continue;
					}
				}
				list2.insertLast(item);
			}

			ServerListModel model(Manager, list2);
			@serverList.Model = model;
			@model.ItemActivated = ServerListItemEventHandler(this.ServerListItemActivated);
			@model.ItemDoubleClicked = ServerListItemEventHandler(this.ServerListItemDoubleClicked);
			@model.ItemRightClicked = ServerListItemEventHandler(this.ServerListItemRightClicked);
			serverList.ScrollToTop();
		}

		private void CheckServerList() {
			if(helper.PollServerListState()) {
				MainScreenServerItem@[]@ list = helper.GetServerList("", false);
				if(list is null or list.length == 0) {
					// failed.
					// FIXME: show error message?
					loaded = false; loading = false;
					errorView.Visible = true;
					loadingView.Visible = false;
					@serverList.Model = spades::ui::ListViewModel(); // empty
					return;
				}
				loading = false;
				loaded = true;
				errorView.Visible = false;
				loadingView.Visible = false;
				UpdateServerList();
			}
		}

		private void OnAddressChanged(spades::ui::UIElement@ sender) {
			cg_lastQuickConnectHost = addressField.Text;
		}

		private void SetProtocolVersion(int ver) {
			protocol3Button.Toggled = (ver == 3);
			protocol4Button.Toggled = (ver == 4);
			cg_protocolVersion = ver;
		}

		private void OnProtocol3Pressed(spades::ui::UIElement@ sender) {
			SetProtocolVersion(3);
		}

		private void OnProtocol4Pressed(spades::ui::UIElement@ sender) {
			SetProtocolVersion(4);
		}

		private void OnFilterProtocol3Pressed(spades::ui::UIElement@ sender) {
			filterProtocol4Button.Toggled = false;
			UpdateServerList();
		}
		private void OnFilterProtocol4Pressed(spades::ui::UIElement@ sender) {
			filterProtocol3Button.Toggled = false;
			UpdateServerList();
		}
		private void OnFilterFullPressed(spades::ui::UIElement@ sender) {
			filterEmptyButton.Toggled = false;
			UpdateServerList();
		}
		private void OnFilterEmptyPressed(spades::ui::UIElement@ sender) {
			filterFullButton.Toggled = false;
			UpdateServerList();
		}
		private void OnFilterTextChanged(spades::ui::UIElement@ sender) {
			UpdateServerList();
		}

		private void OnRefreshServerListPressed(spades::ui::UIElement@ sender) {
			LoadServerList();
		}

		private void OnQuitPressed(spades::ui::UIElement@ sender) {
			ui.shouldExit = true;
		}

		private void OnCreditsPressed(spades::ui::UIElement@ sender) {
			AlertScreen al(this, ui.helper.Credits, Min(500.f, Manager.Renderer.ScreenHeight - 100.f));
			al.Run();
		}

		private void OnSetupPressed(spades::ui::UIElement@ sender) {
			PreferenceView al(this, PreferenceViewOptions(), ui.fontManager);
			al.Run();
		}

		private void Connect() {
			string msg = helper.ConnectServer(addressField.Text, cg_protocolVersion.IntValue);
			if(msg.length > 0) {
				// failde to initialize client.
				AlertScreen al(this, msg);
				al.Run();
			}
		}

		private void OnConnectPressed(spades::ui::UIElement@ sender) {
			Connect();
		}

		void HotKey(string key) {
			if(IsEnabled and key == "Enter") {
				Connect();
			} else if(IsEnabled and key == "Escape") {
				ui.shouldExit = true;
			} else {
				UIElement::HotKey(key);
			}
		}

		void Render() {
			CheckServerList();
			UIElement::Render();

			// check for client error message.
			if(IsEnabled) {
				string msg = helper.GetPendingErrorMessage();
				if(msg.length > 0) {
					// try to maek the "disconnected" message more friendly.
					if(msg.findFirst("Disconnected:") >= 0) {
						int ind1 = msg.findFirst("Disconnected:");
						int ind2 = msg.findFirst("\n", ind1);
						if(ind2 < 0) ind2 = msg.length;
						ind1 += "Disconnected:".length;
						msg = msg.substr(ind1, ind2 - ind1);
						msg = _Tr("MainScreen", "You were disconnected from the server because of the following reason:\n\n{0}", msg);
					}

					// failed to connect.
					AlertScreen al(this, msg);
					al.Run();
				}
			}
		}
	}

	class MainScreenServerListLoadingView: spades::ui::UIElement {
		MainScreenServerListLoadingView(spades::ui::UIManager@ manager) {
			super(manager);
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Font@ font = this.Font;
			string text = _Tr("MainScreen", "Loading...");
			Vector2 txtSize = font.Measure(text);
			Vector2 txtPos;
			txtPos = pos + (size - txtSize) * 0.5f;

			font.Draw(text, txtPos, 1.f, Vector4(1,1,1,0.8));
		}
	}

	class MainScreenServerListErrorView: spades::ui::UIElement {
		MainScreenServerListErrorView(spades::ui::UIManager@ manager) {
			super(manager);
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Font@ font = this.Font;
			string text = _Tr("MainScreen", "Failed to fetch the server list.");
			Vector2 txtSize = font.Measure(text);
			Vector2 txtPos;
			txtPos = pos + (size - txtSize) * 0.5f;

			font.Draw(text, txtPos, 1.f, Vector4(1,1,1,0.8));
		}
	}

	MainScreenUI@ CreateMainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice,
		FontManager@ fontManager, MainScreenHelper@ helper) {
		return MainScreenUI(renderer, audioDevice, fontManager, helper);
	}
}
