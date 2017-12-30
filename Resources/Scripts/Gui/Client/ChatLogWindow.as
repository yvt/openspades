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
#include "ChatSayWindow.as"

namespace spades {

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

}
