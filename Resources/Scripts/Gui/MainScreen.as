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
	

	class MainScreenUI {
		private Renderer@ renderer;
		private AudioDevice@ audioDevice;
		private Font@ font;
		MainScreenHelper@ helper;
		
		spades::ui::UIManager@ manager;
		
		MainScreenMainMenu@ mainMenu;
		
		bool shouldExit = false;
		
		private float time = -1.f;
		
		MainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, Font@ font, MainScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.font = font;
			@this.helper = helper;
			
			SetupRenderer();
			
			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = font;
			
			@mainMenu = MainScreenMainMenu(this);
			mainMenu.Bounds = manager.RootElement.Bounds;
			manager.RootElement.AddChild(mainMenu);
			
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
			sceneDef.depthOfFieldNearRange = 100.f;
			sceneDef.skipWorld = false;
			
			renderer.StartScene(sceneDef);
			renderer.EndScene();
			
			// fade the map
			float fade = Clamp((time - 1.f) / 2.2f, 0.f, 1.f);
			if(fade < 1.f) {
				renderer.Color = Vector4(0.f, 0.f, 0.f, 1.f - fade);
				renderer.DrawImage(renderer.RegisterImage("Gfx/White.tga"),
					AABB2(0.f, 0.f, renderer.ScreenWidth, renderer.ScreenHeight));
			}
			
			// draw title logo
			Image@ img = renderer.RegisterImage("Gfx/Title/Logo.png");
			renderer.Color = Vector4(1.f, 1.f, 1.f, 1.f);
			renderer.DrawImage(img, Vector2((renderer.ScreenWidth - img.Width) * 0.5f, 64.f));
			
			manager.RunFrame(dt);
			manager.Render();
			
			renderer.FrameDone();
			renderer.Flip();
			time += Min(dt, 0.05f);
		}
		
		void Closing() {
			
		}
		
		bool WantsToBeClosed() {
			return shouldExit;
		}
	}
	
	class ServerListItem: spades::ui::ButtonBase {
		MainScreenServerItem@ item;
		ServerListItem(spades::ui::UIManager@ manager, MainScreenServerItem@ item){
			super(manager);
			@this.item = item;
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Image@ img = renderer.RegisterImage("Gfx/White.tga");
			if(Pressed && Hover) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.3f);
			} else if(Hover) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.15f);
			} else {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.0f);
			}
			renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
			
			Font.Draw(item.Name, ScreenPosition + Vector2(4.f, 2.f), 1.f, Vector4(1,1,1,1));
			string playersStr = ToString(item.NumPlayers) + "/" + ToString(item.MaxPlayers);
			Font.Draw(playersStr, ScreenPosition + Vector2(340.f-Font.Measure(playersStr).x * 0.5f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.MapName, ScreenPosition + Vector2(420.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.GameMode, ScreenPosition + Vector2(550.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.Protocol, ScreenPosition + Vector2(650.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw(item.Country, ScreenPosition + Vector2(700.f, 2.f), 1.f, Vector4(1,1,1,1));
		}
	}

	funcdef void ServerListItemEventHandler(ServerListModel@ sender, MainScreenServerItem@ item);
	
	class ServerListModel: spades::ui::ListViewModel {
		spades::ui::UIManager@ manager;
		MainScreenServerItem@[]@ list;
		
		ServerListItemEventHandler@ ItemActivated;
		
		ServerListModel(spades::ui::UIManager@ manager, MainScreenServerItem@[]@ list) {
			@this.manager = manager;
			@this.list = list;
		}
		int NumRows { 
			get { return int(list.length); }
		}
		private void ItemClicked(spades::ui::UIElement@ sender){
			ServerListItem@ item = cast<ServerListItem>(sender);
			if(ItemActivated !is null) {
				ItemActivated(this, item.item);
			}
		}
		spades::ui::UIElement@ CreateElement(int row) {
			ServerListItem i(manager, list[row]);
			@i.Activated = spades::ui::EventHandler(this.ItemClicked);
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
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.3f);
			} else if(Hover) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.15f);
			} else {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.0f);
			}
			renderer.DrawImage(img, AABB2(pos.x - 2.f, pos.y, size.x, size.y));
			
			Font.Draw(Text, ScreenPosition + Vector2(0.f, 2.f), 1.f, Vector4(1,1,1,1));
		}
	}
	
	
	class ProtocolButton: spades::ui::Button {
		ProtocolButton(spades::ui::UIManager@ manager){
			super(manager);
			Toggle = true;
		}
		void Render() {
			Renderer@ renderer = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Image@ img = renderer.RegisterImage("Gfx/White.tga");
			if((Pressed && Hover) || Toggled) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.2f);
			} else if(Hover) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.12f);
			} else {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.07f);
			}
			renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
			if((Pressed && Hover) || Toggled) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.1f);
			} else if(Hover) {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.07f);
			} else {
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.03f);
			}
			renderer.DrawImage(img, AABB2(pos.x, pos.y, 1.f, size.y));
			renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
			renderer.DrawImage(img, AABB2(pos.x+size.x-1.f, pos.y, 1.f, size.y));
			renderer.DrawImage(img, AABB2(pos.x, pos.y+size.y-1.f, size.x, 1.f));
			Vector2 txtSize = Font.Measure(Caption);
			Font.DrawShadow(Caption, pos + (size - txtSize) * 0.5f, 1.f, Vector4(1,1,1,1), Vector4(0,0,0,0.4f));
		}
	}
	
	class MainScreenMainMenu: spades::ui::UIElement {
		
		MainScreenUI@ ui;
		MainScreenHelper@ helper;
		spades::ui::Field@ addressField;
		
		spades::ui::Button@ protocol3Button;
		spades::ui::Button@ protocol4Button;
		
		spades::ui::ListView@ serverList;
		MainScreenServerListLoadingView@ loadingView;
		MainScreenServerListErrorView@ errorView;
		bool loading = false, loaded = false;
		
		private ConfigItem cg_protocolVersion("cg_protocolVersion");
		private ConfigItem cg_lastQuickConnectHost("cg_lastQuickConnectHost");
		private ConfigItem cg_serverlistSort("cg_serverlistSort", "16385");
		
		MainScreenMainMenu(MainScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			@this.helper = ui.helper;
			
			float contentsWidth = 750.f;
			float contentsLeft = (Manager.Renderer.ScreenWidth - contentsWidth) * 0.5f;
			float footerPos = Manager.Renderer.ScreenHeight - 40.f;
			{
				spades::ui::Button button(Manager);
				button.Caption = "Connect";
				button.Bounds = AABB2(contentsLeft + contentsWidth - 150.f, 200.f, 150.f, 30.f);
				button.Activated = EventHandler(this.OnConnectPressed);
				AddChild(button);
			}
			{
				@addressField = spades::ui::Field(Manager);
				addressField.Bounds = AABB2(contentsLeft, 200, contentsWidth - 240.f, 30.f);
				addressField.Placeholder = "Quick Connect";
				addressField.Text = cg_lastQuickConnectHost.StringValue;
				addressField.Changed = spades::ui::EventHandler(this.OnAddressChanged);
				AddChild(addressField);
			}
			{
				@protocol3Button = ProtocolButton(Manager);
				protocol3Button.Bounds = AABB2(contentsLeft + contentsWidth - 240.f + 6.f, 200, 
					40.f, 30.f);
				protocol3Button.Caption = "0.75";
				protocol3Button.Activated = spades::ui::EventHandler(this.OnProtocol3Pressed);
				protocol3Button.Toggle = true;
				protocol3Button.Toggled = cg_protocolVersion.IntValue == 3;
				AddChild(protocol3Button);
			}
			{
				@protocol4Button = ProtocolButton(Manager);
				protocol4Button.Bounds = AABB2(contentsLeft + contentsWidth - 200.f + 6.f, 200, 
					40.f, 30.f);
				protocol4Button.Caption = "0.76";
				protocol4Button.Activated = spades::ui::EventHandler(this.OnProtocol4Pressed);
				protocol4Button.Toggle = true;
				protocol4Button.Toggled = cg_protocolVersion.IntValue == 4;
				AddChild(protocol4Button);
			}
			{
				@serverList = spades::ui::ListView(Manager);
				serverList.Bounds = AABB2(contentsLeft, 270.f, contentsWidth, footerPos - 290.f);
				AddChild(serverList);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 2.f, 240.f, 300.f - 2.f, 30.f);
				header.Text = "Server Name";
				header.Activated = spades::ui::EventHandler(this.SortServerListByName);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 300.f, 240.f, 120.f, 30.f);
				header.Text = "Players";
				header.Activated = spades::ui::EventHandler(this.SortServerListByNumPlayers);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 420.f, 240.f, 130.f, 30.f);
				header.Text = "Map Name";
				header.Activated = spades::ui::EventHandler(this.SortServerListByMapName);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 550.f, 240.f, 100.f, 30.f);
				header.Text = "Game Mode";
				header.Activated = spades::ui::EventHandler(this.SortServerListByGameMode);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 650.f, 240.f, 50.f, 30.f);
				header.Text = "Ver.";
				header.Activated = spades::ui::EventHandler(this.SortServerListByProtocol);
				AddChild(header);
			}
			{
				ServerListHeader header(Manager);
				header.Bounds = AABB2(contentsLeft + 700.f, 240.f, 50.f, 30.f);
				header.Text = "Loc.";
				header.Activated = spades::ui::EventHandler(this.SortServerListByCountry);
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
			if(list is null){
				@serverList.Model = spades::ui::ListViewModel(); // empty
				return;
			}
			ServerListModel model(Manager, list);
			@serverList.Model = model;
			@model.ItemActivated = ServerListItemEventHandler(this.ServerListItemActivated);
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
		
		
		private void Connect() {
			string msg = helper.ConnectServer();
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
			string text = "Loading...";
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
			string text = "Failed to fetch the server list.";
			Vector2 txtSize = font.Measure(text);
			Vector2 txtPos;
			txtPos = pos + (size - txtSize) * 0.5f;
			
			font.Draw(text, txtPos, 1.f, Vector4(1,1,1,0.8));
		}
	}
	
	MainScreenUI@ CreateMainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, MainScreenHelper@ helper) {
		return MainScreenUI(renderer, audioDevice, font, helper);
	}
}
