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
    class ClientLimboWindow: spades::ui::UIElement {
        private ClientUI@ ui;
        private ClientUIHelper@ helper;

        private spades::ui::Button@[] teamButtons;
        private spades::ui::Button@[] weaponButtons;
        private spades::ui::Button@ spawnButton;
        private spades::ui::Button@ cancelButton;

        private int selectedTeam = -1;
        private int selectedWeapon = -1;

        spades::ui::EventHandler@ Done;
        spades::ui::EventHandler@ Cancelled;

        ClientLimboWindow(ClientUI@ ui) {
            super(ui.manager);
            @this.ui = ui;
            @this.helper = ui.helper;

            Bounds = AABB2(0, 0, 860, 200);

            // Background
            {
                spades::ui::Label label(Manager);
                label.BackgroundColor = Vector4(0, 0, 0, 0.2f);
                label.Bounds = AABB2(0, 0, Size.x, Size.y);
                AddChild(label);
            }

            // Teams
            {
                spades::ui::Label label(Manager);
                label.Text = _Tr("Client", "Select a team:");
                label.Bounds = AABB2(60, 10, 0, 0);
                @label.Font = ui.fontManager.SmallerFont;
                AddChild(label);
            }
            for (int i = 0; i < 3; ++i) {
                spades::ui::Button button(Manager);
                button.Bounds = AABB2(60, 40 + 50 * i, 200, 40);
                @button.Font = ui.fontManager.MediumFont;
                @button.Activated = spades::ui::EventHandler(this.OnButtonActivated);
                AddChild(button);
                teamButtons.insertLast(button);
            }
            teamButtons[2].Caption = _Tr("Client", "Spectator");

            // Weapons
            {
                spades::ui::Label label(Manager);
                label.Text = _Tr("Client", "Select a weapon:");
                label.Bounds = AABB2(330, 10, 0, 0);
                @label.Font = ui.fontManager.SmallerFont;
                AddChild(label);
            }
            for (int i = 0; i < 3; ++i) {
                spades::ui::Button button(Manager);
                button.Bounds = AABB2(330, 40 + 50 * i, 200, 40);
                @button.Font = ui.fontManager.MediumFont;
                @button.Activated = spades::ui::EventHandler(this.OnButtonActivated);
                AddChild(button);
                weaponButtons.insertLast(button);
            }
            weaponButtons[0].Caption = _Tr("Client", "Rifle");
            weaponButtons[1].Caption = _Tr("Client", "SMG");
            weaponButtons[2].Caption = _Tr("Client", "Shotgun");

            // Spawn!
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("Client", "Spawn");
                button.Bounds = AABB2(600, 90, 200, 40);
                @button.Font = ui.fontManager.MediumFont;
                @button.Activated = spades::ui::EventHandler(this.OnButtonActivated);
                AddChild(button);
                @spawnButton = button;
            }

            // Cancel
            {
                spades::ui::Button button(Manager);
                button.Caption = "X"; // TODO: use better close icon
                button.Bounds = AABB2(Size.x - 60, 0, 60, 25);
                button.Alignment = Vector2(0.5, 0.5);
                @button.Activated = spades::ui::EventHandler(this.OnButtonActivated);
                AddChild(button);
                @cancelButton = button;
            }
            {
                spades::ui::Label label(Manager);
                label.Text = "[Escape]";
                label.Bounds = AABB2(-8, 0, 0, 25);
                label.Alignment = Vector2(1, 0.5);
                label.TextColor = Vector4(1, 1, 1, 0.5);
                @label.Font = ui.fontManager.SmallerFont;
                cancelButton.AddChild(label);
            }

            UpdateButtonState();
        }

        void BeforeAppear() {
            selectedTeam = helper.LocalPlayerTeamId;
            selectedWeapon = helper.LocalPlayerWeaponId;
            teamButtons[0].Caption = helper.GetTeamName(0);
            teamButtons[1].Caption = helper.GetTeamName(1);
            UpdateButtonState();
        }

        private int GetActiveColumn() {
            if (selectedTeam == -1) {
                return 0;
            } else {
                return 1;
            }
        }

        private bool IsReadyToSpawn() {
            return selectedTeam == 2 || (selectedTeam != -1 && selectedWeapon != -1);
        }

        private void SendSpawn() {
            Done(this);
            helper.Spawn(selectedTeam, selectedWeapon);
        }

        private void UpdateButtonState() {
            int activeColumn = GetActiveColumn();

            for (int i = 0; i < 3; ++i) {
                if (activeColumn == 0) {
                    teamButtons[i].HotKeyText = "[" + ToString(i + 1) + "]";
                } else {
                    teamButtons[i].HotKeyText = "";
                }
                teamButtons[i].Toggled = (i == selectedTeam);
                if (activeColumn == 1) {
                    weaponButtons[i].HotKeyText = "[" + ToString(i + 1) + "]";
                } else {
                    weaponButtons[i].HotKeyText = "";
                }
                weaponButtons[i].Toggled = (i == selectedWeapon);
            }
            if (activeColumn == 0) {
                spawnButton.HotKeyText = "[3]";
            } else {
                spawnButton.HotKeyText = "[1,2,3]";
            }
            spawnButton.Enable = IsReadyToSpawn();
            cancelButton.Visible = helper.HasLocalPlayer;
        }

        private void OnButtonActivated(spades::ui::UIElement@ sender) {
            for (int i = 0; i < 3; ++i) {
                if (teamButtons[i] is sender) {
                    selectedTeam = i;
                    UpdateButtonState();
                    return;
                }
            }
            for (int i = 0; i < 3; ++i) {
                if (weaponButtons[i] is sender) {
                    selectedWeapon = i;
                    UpdateButtonState();
                    return;
                }
            }
            if (spawnButton is sender) {
                if (IsReadyToSpawn()) {
                    SendSpawn();
                }
                return;
            }
            if (cancelButton is sender) {
                Cancelled(this);
                return;
            }
        }

        void HotKey(string key) {
            if (IsEnabled) {
                if (key == "1") {
                    OnButtonActivated(GetActiveColumn() == 0 ? teamButtons[0] : weaponButtons[0]);
                    if (IsReadyToSpawn()) {
                        SendSpawn();
                    }
                    return;
                } else if (key == "2") {
                    OnButtonActivated(GetActiveColumn() == 0 ? teamButtons[1] : weaponButtons[1]);
                    if (IsReadyToSpawn()) {
                        SendSpawn();
                    }
                    return;
                } else if (key == "3") {
                    OnButtonActivated(GetActiveColumn() == 0 ? teamButtons[2] : weaponButtons[2]);
                    if (IsReadyToSpawn()) {
                        SendSpawn();
                    }
                    return;
                } else if (key == "Escape") {
                    Cancelled(this);
                }
            }
            UIElement::HotKey(key);
        }

        void Render() {
            Manager.PushClippingRect(ScreenBounds);
            Manager.Renderer.Blur();
            Manager.PopClippingRect();
            UIElement::Render();
        }
    }
}