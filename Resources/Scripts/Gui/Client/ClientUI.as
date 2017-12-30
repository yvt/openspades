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
#include "Menu.as"
#include "FieldWithHistory.as"
#include "ChatLogWindow.as"

namespace spades {

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

    ClientUI@ CreateClientUI(Renderer@ renderer, AudioDevice@ audioDevice,
        FontManager@ fontManager, ClientUIHelper@ helper) {
        return ClientUI(renderer, audioDevice, fontManager, helper);
    }

}
