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
#include "ClientUI.as"

namespace spades {

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

}
