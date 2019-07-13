/*
 Copyright (c) 2017 yvt

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

#include "../UIFramework/UIControls.as"

namespace spades {
    class UpdateCheckView : spades::ui::UIElement {
        spades::ui::EventHandler @OpenUpdateInfoURL;

        private PackageUpdateManager @packageUpdateManager;

        private spades::ui::Button @enableCheckButton;
        private spades::ui::Button @showDetailsButton;

        private ConfigItem cl_checkForUpdates("cl_checkForUpdates");

        UpdateCheckView(spades::ui::UIManager @manager,
                        PackageUpdateManager @packageUpdateManager) {
            super(manager);
            @this.packageUpdateManager = packageUpdateManager;

            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("UpdateCheck", "Enable");
                @button.Activated = spades::ui::EventHandler(this.OnEnableCheckPressed);
                @enableCheckButton = button;
                AddChild(button);
            }
            {
                spades::ui::Button button(Manager);
                button.Caption = _Tr("UpdateCheck", "Show details...");
                @button.Activated = spades::ui::EventHandler(this.OnShowDetailsPressed);
                @showDetailsButton = button;
                AddChild(button);
            }
        }

        void Render() {
            PackageUpdateManagerReadyState state = packageUpdateManager.UpdateInfoReadyState;

            float viewWidth = Size.x;
            float viewHeight = Size.y;
            enableCheckButton.Bounds =
                AABB2(viewWidth - 160.f, (viewHeight - 30.f) * 0.5f, 150.f, 30.f);
            showDetailsButton.Bounds =
                AABB2(viewWidth - 160.f, (viewHeight - 30.f) * 0.5f, 150.f, 30.f);

            Renderer @renderer = Manager.Renderer;
            Vector2 pos = ScreenPosition;
            Vector2 size = Size;
            Image @white = renderer.RegisterImage("Gfx/White.tga");

            Vector4 background = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            switch (state) {
                case PackageUpdateManagerReadyState::Loaded:
                    if (packageUpdateManager.UpdateAvailable) {
                        float pulse = sin(Manager.Time * 2.f);
                        pulse = abs(pulse);
                        background = Vector4(0.0f, 0.1f + pulse * 0.2f, 0.0f, 1.0f);
                    }
                    break;
                case PackageUpdateManagerReadyState::Error:
                    background = Vector4(0.3f, 0.0f, 0.0f, 1.0f);
                    break;
                case PackageUpdateManagerReadyState::NotLoaded:
                    background = Vector4(0.3f, 0.3f, 0.0f, 1.0f);
                    break;
            }
            renderer.ColorNP = background;
            renderer.DrawImage(white, AABB2(pos.x, pos.y, size.x, size.y));

            if (state == PackageUpdateManagerReadyState::Loading) {
                float phase = fraction(Manager.Time) * 80.0f;
                renderer.ColorNP = Vector4(0.15f, 0.15f, 0.15f, 1.0f);
                for (float x = phase - 160.0f; x < size.x + 160.0f; x += 80.0f) {
                    renderer.DrawImage(white, Vector2(x, pos.y), Vector2(x + 40.f, pos.y),
                                       Vector2(x - 20.f, pos.y + size.y), AABB2(0, 0, 0, 0));
                }
            }

            string text;

            switch (state) {
                case PackageUpdateManagerReadyState::Loaded:
                    if (packageUpdateManager.UpdateAvailable) {
                        text =
                            _Tr("UpdateCheck", "Version {0} is available! (You currently have {1})",
                                packageUpdateManager.LatestVersionInfo.Text,
                                packageUpdateManager.CurrentVersionInfo.Text);
                    } else {
                        text = _Tr("UpdateCheck",
                                   "You're using the latest version of OpenSpades. ({0})",
                                   packageUpdateManager.CurrentVersionInfo.Text);
                    }
                    break;
                case PackageUpdateManagerReadyState::Loading:
                    text = _Tr("UpdateCheck", "Checking for updates...");
                    break;
                case PackageUpdateManagerReadyState::Error:
                    text = _Tr("UpdateCheck", "Failed to check for updates.");
                    break;
                case PackageUpdateManagerReadyState::NotLoaded:
                    text = _Tr("UpdateCheck", "Automatic update check is not enabled.");
                    break;
                case PackageUpdateManagerReadyState::Unavailable:
                    text = _Tr("UpdateCheck", "Automatic update check is not available.");
                    break;
            }

            Font @font = this.Font;
            Vector2 txtSize = font.Measure(text);
            Vector2 txtPos = pos + (size - txtSize) * Vector2(0.0f, 0.5f) + Vector2(10.0f, 0.0f);
            font.Draw(text, txtPos, 1.0f, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

            enableCheckButton.Visible = false;
            showDetailsButton.Visible = false;
            switch (state) {
                case PackageUpdateManagerReadyState::Loaded:
                    if (packageUpdateManager.UpdateAvailable &&
                        packageUpdateManager.LatestVersionInfoPageURL != "") {
                        showDetailsButton.Visible = true;
                    }
                    break;
                case PackageUpdateManagerReadyState::NotLoaded:
                    enableCheckButton.Visible = true;
                    break;
            }

            UIElement::Render();
        }

        private void OnEnableCheckPressed(spades::ui::UIElement @) {
            cl_checkForUpdates.IntValue = 1;
            packageUpdateManager.CheckForUpdate();
        }
        private void OnShowDetailsPressed(spades::ui::UIElement @) { OpenUpdateInfoURL(this); }
    }
}
