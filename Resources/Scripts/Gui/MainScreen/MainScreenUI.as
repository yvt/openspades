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

#include "MainMenu.as"
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

    /**
     * The entry point of the main screen.
     */
    MainScreenUI@ CreateMainScreenUI(Renderer@ renderer, AudioDevice@ audioDevice,
        FontManager@ fontManager, MainScreenHelper@ helper) {
        return MainScreenUI(renderer, audioDevice, fontManager, helper);
    }

}
