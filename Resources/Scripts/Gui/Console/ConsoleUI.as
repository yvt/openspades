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
#include "ConsoleWindow.as"

namespace spades {
    ConsoleUI @CreateConsoleUI(Renderer @renderer, AudioDevice @audioDevice,
                               FontManager @fontManager, ConsoleHelper @helper) {
        return ConsoleUI(renderer, audioDevice, fontManager, helper);
    }

    /**
     * Implements the system console window.
     *
     * `ConsoleUI` is overlaid on the normal rendering. Normally, it's invisible,
     * but it becomes visible when invoked by a hotkey. When it's visible, it
     * also intercepts all inputs.
     */
    class ConsoleUI {
        private spades::ui::UIManager @manager;
        private bool active = false;

        private ConsoleWindow @console;

        ConsoleUI(Renderer @renderer, AudioDevice @audioDevice, FontManager @fontManager,
                  ConsoleHelper @helper) {
            @manager = spades::ui::UIManager(renderer, audioDevice);
            @manager.RootElement.Font = fontManager.GuiFont;

            @console = ConsoleWindow(helper, manager);
            console.Bounds = manager.RootElement.Bounds;
            manager.RootElement.AddChild(console);
        }

        void MouseEvent(float x, float y) { manager.MouseEvent(x, y); }

        void WheelEvent(float x, float y) { manager.WheelEvent(x, y); }

        void KeyEvent(string key, bool down) {
            if (key == "Escape") {
                active = false;
                return;
            }
            manager.KeyEvent(key, down);
        }

        void TextInputEvent(string text) { manager.TextInputEvent(text); }

        void TextEditingEvent(string text, int start, int len) {
            manager.TextEditingEvent(text, start, len);
        }

        bool AcceptsTextInput() { return manager.AcceptsTextInput; }

        AABB2 GetTextInputRect() { return manager.TextInputRect; }

        void RunFrame(float dt) {
            manager.RunFrame(dt);
            if (active) {
                manager.Render();
            }
        }

        void Closing() {}

        bool ShouldInterceptInput() { return active; }

        void ToggleConsole() {
            active = !active;
            if (active) {
                console.FocusField();
            }
        }

        void AddLine(string line) { console.AddLine(line); }
    }
}
