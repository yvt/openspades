/*
 Copyright (c) 2019 yvt

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
#include "../UIFramework/UIFramework.as"
#include "ConsoleCommandField.as"

namespace spades {
    class ConsoleWindow : spades::ui::UIElement {
        private ConsoleHelper @helper;
        private array<spades::ui::CommandHistoryItem @> history = {};
        private FieldWithHistory @field;
        private spades::ui::TextViewer @viewer;

        private ConfigItem cl_consoleScrollbackLines("cl_consoleScrollbackLines", "1000");

        ConsoleWindow(ConsoleHelper @helper, spades::ui::UIManager @manager) {
            super(manager);
            @this.helper = helper;

            float screenWidth = Manager.Renderer.ScreenWidth;
            float screenHeight = Manager.Renderer.ScreenHeight;
            float height = floor(screenHeight * 0.4);

            {
                spades::ui::Label label(Manager);
                label.BackgroundColor = Vector4(0, 0, 0, 0.8f);
                label.Bounds = AABB2(0.f, 0.f, Manager.Renderer.ScreenWidth, height);
                AddChild(label);
            }
            {
                spades::ui::Label label(Manager);
                label.BackgroundColor = Vector4(0, 0, 0, 0.5f);
                label.Bounds = AABB2(0.f, height, Manager.Renderer.ScreenWidth,
                                     Manager.Renderer.ScreenHeight - height);
                AddChild(label);
            }

            {
                @field = ConsoleCommandField(Manager, this.history, helper);
                field.Bounds = AABB2(10.0, height - 35.0, screenWidth - 20.0, 30.f);
                field.Placeholder = _Tr("Console", "Command");
                @field.Changed = spades::ui::EventHandler(this.OnFieldChanged);
                AddChild(field);
            }
            {
                spades::ui::TextViewer viewer(Manager);
                AddChild(viewer);
                viewer.Bounds = AABB2(10.0, 5.0, screenWidth - 20.0, height - 45.0);
                viewer.MaxNumLines = uint(cl_consoleScrollbackLines.IntValue);
                @this.viewer = viewer;
            }
        }

        private void OnFieldChanged(spades::ui::UIElement @sender) {}

        void FocusField() { @Manager.ActiveElement = field; }

        void AddLine(string line) { viewer.AddLine(line, true); }

        void HotKey(string key) {
            if (key == "Enter") {
                if (field.Text.length == 0) {
                    return;
                }
                field.CommandSent();
                helper.ExecCommand(field.Text);
                field.Clear();
            }
        }
    }
} // namespace spades
