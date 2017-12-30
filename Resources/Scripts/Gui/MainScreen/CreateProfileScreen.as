/*
 Copyright (c) 2016 yvt

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
    class CreateProfileScreen: spades::ui::UIElement {

        private float contentsTop, contentsHeight;

        spades::ui::EventHandler@ Closed;
        private spades::ui::UIElement@ owner;
        private spades::ui::Field@ nameField;
        private spades::ui::Button@ okButton;

        private ConfigItem cg_playerName("cg_playerName", "Deuce");
        private ConfigItem cg_playerNameIsSet("cg_playerNameIsSet", "0");

        CreateProfileScreen(spades::ui::UIElement@ owner) {
            super(owner.Manager);
            @this.owner = owner;
            @Font = Manager.RootElement.Font;
            this.Bounds = owner.Bounds;

            float contentsWidth = 500.f;
            float contentsLeft = (Manager.Renderer.ScreenWidth - contentsWidth) * 0.5f;
            contentsHeight = 188.f;
            contentsTop = (Manager.Renderer.ScreenHeight - contentsHeight) * 0.5f;
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
                button.Caption = _Tr("CreateProfileScreen", "OK");
                button.Bounds = AABB2(
                    contentsLeft + contentsWidth - 140.f,
                    contentsTop + contentsHeight - 40.f
                    , 140.f, 30.f);
                @button.Activated = spades::ui::EventHandler(this.OnOkPressed);
                button.Enable = false;
                AddChild(button);
                @okButton = button;
            }
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("CreateProfileScreen", "Decide later");
                button.Bounds = AABB2(
                    contentsLeft,
                    contentsTop + contentsHeight - 40.f
                    , 140.f, 30.f);
                @button.Activated = spades::ui::EventHandler(this.OnChooseLater);
                AddChild(button);
            }
            {
                spades::ui::Label label(Manager);
                label.Text = _Tr("CreateProfileScreen", "Welcome to OpenSpades");
                label.TextScale = 1.3f;
                label.Bounds = AABB2(contentsLeft, contentsTop + 10.f, contentsWidth, 32.f);
                label.Alignment = Vector2(0.f, 0.5f);
                AddChild(label);
            }
            {
                spades::ui::Label label(Manager);
                label.Text = _Tr("CreateProfileScreen", "Choose a player name:");
                label.Bounds = AABB2(contentsLeft, contentsTop + 42.f, contentsWidth, 32.f);
                label.Alignment = Vector2(0.f, 0.5f);
                AddChild(label);
            }
            {
                @nameField = spades::ui::Field(Manager);
                nameField.Bounds = AABB2(contentsLeft, contentsTop + 74.f, contentsWidth, 30.f);
                nameField.Placeholder = _Tr("CreateProfileScreen", "Player name");
                @nameField.Changed = spades::ui::EventHandler(this.OnNameChanged);
                nameField.MaxLength = 15;
                nameField.DenyNonAscii = true;
                AddChild(nameField);
            }
            {
                spades::ui::Label label(Manager);
                label.Text = _Tr("CreateProfileScreen", "You can change it later in the Setup dialog.");
                label.Bounds = AABB2(contentsLeft, contentsTop + 106.f, contentsWidth, 32.f);
                label.Alignment = Vector2(0.f, 0.5f);
                AddChild(label);
            }
        }

        private void OnClosed() {
            if(Closed !is null) {
                Closed(this);
            }
        }

        void Close() {
            owner.Enable = true;
            owner.Parent.RemoveChild(this);
        }

        void Run() {
            owner.Enable = false;
            owner.Parent.AddChild(this);

            @Manager.ActiveElement = nameField;
        }

        private void OnOkPressed(spades::ui::UIElement@ sender) {
            if (nameField.Text.length == 0) {
                return;
            }
            cg_playerName = nameField.Text;
            cg_playerNameIsSet = 1;
            Close();
        }

        private void OnChooseLater(spades::ui::UIElement@ sender) {
            Close();
        }

        private void OnNameChanged(spades::ui::UIElement@ sender) {
            okButton.Enable = nameField.Text.length > 0;
        }

        void HotKey(string key) {
            if(IsEnabled and (key == "Enter" or key == "Escape")) {
                if (key == "Enter") {
                    OnOkPressed(this);
                } else if (key == "Escape") {
                    OnChooseLater(this);
                }
            } else {
                UIElement::HotKey(key);
            }
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
