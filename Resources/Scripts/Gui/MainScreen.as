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
		
		private spades::ui::UIManager@ manager;
		
		private bool shouldExit = false;
		
		private float time = 0.f;
		
		MainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, Font@ font, MainScreenHelper@ helper) {
			@this.renderer = renderer;
			@this.audioDevice = audioDevice;
			@this.font = font;
			@this.helper = helper;
			
			// load map
			@renderer.GameMap = GameMap("Maps/mesa.vxl");
			renderer.FogColor = Vector3(0.11f, 0.10f, 0.2f);
			renderer.FogDistance = 128.f;
			
			@manager = spades::ui::UIManager(renderer, audioDevice);
			@manager.RootElement.Font = font;
			
			spades::ui::Button button(manager);
			button.Caption = "Not implemented! Sorry!";
			button.Bounds = AABB2(200.f, 200.f, 120.f, 32.f);
			manager.RootElement.AddChild(button);
		}
		
		void MouseEvent(float x, float y) {
			manager.MouseEvent(x, y);
		}
		
		void KeyEvent(string key, bool down) {
			if(key == "Escape") {
				shouldExit = true;
				return;
			}
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
			SceneDefinition sceneDef;
			sceneDef = SetupCamera(sceneDef, 
				Vector3(256.f, 256.f, 1.f), Vector3(256.f, 257.f, 2.f), Vector3(0.f, 0.f, -1.f),
				80.f);
			sceneDef.zNear = 1.f;
			sceneDef.zFar = 200.f;
			sceneDef.time = int(time * 1000.f);
			sceneDef.viewportWidth = int(renderer.ScreenWidth);
			sceneDef.viewportHeight = int(renderer.ScreenHeight);
			sceneDef.denyCameraBlur = true;
			
			renderer.StartScene(sceneDef);
			renderer.EndScene();
			
			manager.Render();
			renderer.FrameDone();
			renderer.Flip();
			time += dt;
		}
		
		void Closing() {
			
		}
		
		bool WantsToBeClosed() {
			return shouldExit;
		}
	}
	
	MainScreenUI@ CreateMainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice, 
		Font@ font, MainScreenHelper@ helper) {
		return MainScreenUI(renderer, audioDevice, font, helper);
	}
}
