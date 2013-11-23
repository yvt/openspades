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
		private MainScreenHelper@ helper;
		
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
		
		private void SetupRenderer() {
			// load map
			@renderer.GameMap = GameMap("Maps/Title.vxl");
			renderer.FogColor = Vector3(0.1f, 0.10f, 0.1f);
			renderer.FogDistance = 128.f;
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
		string Text;
		ServerListItem(spades::ui::UIManager@ manager){
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
			renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
			
			Font.Draw(Text, ScreenPosition + Vector2(2.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw("TODO", ScreenPosition + Vector2(100.f, 2.f), 1.f, Vector4(1,1,1,1));
			Font.Draw("Blahblah", ScreenPosition + Vector2(200.f, 2.f), 1.f, Vector4(1,1,1,1));
		}
	}

	class ServerListModel: spades::ui::ListViewModel {
		spades::ui::UIManager@ manager;
		ServerListModel(spades::ui::UIManager@ manager) {
			@this.manager = manager;
		}
		int NumRows { get { return 100; } }
		spades::ui::UIElement@ CreateElement(int row) {
			ServerListItem i(manager);
			// TODO: implement
			i.Text = "Row " + ToString(row);
			return i;
		}
		void RecycleElement(spades::ui::UIElement@ elem) {}
	}
	
	class MainScreenMainMenu: spades::ui::UIElement {
		
		MainScreenUI@ ui;
		spades::ui::Field@ addressField;
		
		spades::ui::ListView@ serverList;
		
		private ConfigItem cg_lastQuickConnectHost("cg_lastQuickConnectHost");
		
		MainScreenMainMenu(MainScreenUI@ ui) {
			super(ui.manager);
			@this.ui = ui;
			
			float contentsWidth = 600.f;
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
				addressField.Bounds = AABB2(contentsLeft, 200, contentsWidth - 160.f, 30.f);
				addressField.Placeholder = "Quick Connect";
				addressField.Text = cg_lastQuickConnectHost.StringValue;
				addressField.Changed = spades::ui::EventHandler(this.OnAddressChanged);
				AddChild(addressField);
			}
			{
				@serverList = spades::ui::ListView(Manager);
				serverList.Bounds = AABB2(contentsLeft, 240.f, contentsWidth, footerPos - 250.f);
				AddChild(serverList);
				@serverList.Model = ServerListModel(Manager);
			}
		}
		
		private void OnAddressChanged(spades::ui::UIElement@ sender) {
			cg_lastQuickConnectHost = addressField.Text;
		}
		
		private void Connect() {
			string text = "This feature is not implemented.";
			text += text;
			text += text;
			text += text;
			text += text;
			text += text;
			text += text;
			AlertScreen al(this, text);
			al.Run();
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
		
		
	}
	
	
	
	MainScreenUI@ CreateMainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, MainScreenHelper@ helper) {
		return MainScreenUI(renderer, audioDevice, font, helper);
	}
}
