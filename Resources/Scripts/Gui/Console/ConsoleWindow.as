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

namespace spades {
    class ConsoleWindow : spades::ui::UIElement {
        private ConsoleHelper @helper;

        ConsoleWindow(ConsoleHelper @helper, spades::ui::UIManager @manager) {
            super(manager);
            @this.helper = helper;

            float height = floor(Manager.Renderer.ScreenHeight * 0.2);

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
            // TODO
        }
    }
} // namespace spades
