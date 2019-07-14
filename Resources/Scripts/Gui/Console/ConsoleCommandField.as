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
#include "../Client/FieldWithHistory.as"

namespace spades {

    uint StringCommonPrefixLength(string a, string b) {
        for (uint i = 0, ln = Min(a.length, b.length); i < ln; i++) {
            if (ToLower(a[i]) != ToLower(b[i]))
                return i;
        }
        return Min(a.length, b.length);
    }

    /** Displys console command candidates. */
    class ConsoleCommandFieldCandiateView : spades::ui::UIElement {
        ConsoleCommandCandidate[] @candidates;
        ConsoleCommandFieldCandiateView(spades::ui::UIManager @manager,
                                        ConsoleCommandCandidate[] @candidates) {
            super(manager);
            @this.candidates = candidates;
        }
        void Render() {
            float maxNameLen = 0.f;
            float maxDescLen = 20.f;
            Font @font = this.Font;
            Renderer @renderer = this.Manager.Renderer;
            float rowHeight = 25.f;

            for (uint i = 0, len = candidates.length; i < len; i++) {
                maxNameLen = Max(maxNameLen, font.Measure(candidates[i].Name).x);
                maxDescLen = Max(maxDescLen, font.Measure(candidates[i].Description).x);
            }
            Vector2 pos = this.ScreenPosition;

            renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.5f);
            renderer.DrawImage(null, AABB2(pos.x, pos.y, maxNameLen + maxDescLen + 20.f,
                                           float(candidates.length) * rowHeight + 10.f));

            for (uint i = 0, len = candidates.length; i < len; i++) {
                font.DrawShadow(candidates[i].Name, pos + Vector2(5.f, 8.f + float(i) * rowHeight),
                                1.f, Vector4(1, 1, 1, 0.7), Vector4(0, 0, 0, 0.3f));
                font.DrawShadow(candidates[i].Description,
                                pos + Vector2(15.f + maxNameLen, 8.f + float(i) * rowHeight), 1.f,
                                Vector4(1, 1, 1, 1), Vector4(0, 0, 0, 0.4f));
            }
        }
    }

    /** A `Field` with a command name autocompletion feature. */
    class ConsoleCommandField : FieldWithHistory {
        private ConsoleCommandFieldCandiateView @valueView;
        private ConsoleHelper @helper;

        ConsoleCommandField(spades::ui::UIManager @manager,
                            array<spades::ui::CommandHistoryItem @> @history,
                            ConsoleHelper @helper) {
            super(manager, history);

            @this.helper = helper;
        }

        void OnChanged() {
            FieldWithHistory::OnChanged();

            if (valueView !is null) {
                @valueView.Parent = null;
            }

            // Find the command name part
            int whitespace = Text.findFirst(" ");
            if (whitespace < 0) {
                whitespace = int(Text.length);
            }

            if (whitespace > 0) {
                string input = Text.substr(0, whitespace);
                ConsoleCommandCandidateIterator @it = helper.AutocompleteCommandName(input);
                ConsoleCommandCandidate[] @candidates = {};

                for (uint i = 0; i < 16; ++i) {
                    if (!it.MoveNext()) {
                        break;
                    }

                    candidates.insertLast(it.Current);
                }

                if (candidates.length > 0) {
                    @valueView = ConsoleCommandFieldCandiateView(this.Manager, candidates);
                    valueView.Bounds = AABB2(0.f, Size.y + 10.f, 0.f, 0.f);
                    @valueView.Parent = this;
                }
            }
        }

        void KeyDown(string key) {
            if (key == "Tab") {
                // Find the command name part
                int whitespace = Text.findFirst(" ");
                if (whitespace < 0) {
                    whitespace = int(Text.length);
                }

                if (SelectionLength == 0 && SelectionStart <= whitespace) {
                    // Command name auto completion
                    string input = Text.substr(0, whitespace);
                    ConsoleCommandCandidateIterator @it = helper.AutocompleteCommandName(input);
                    string commonPart;
                    bool foundOne = false;
                    while (it.MoveNext()) {
                        string name = it.Current.Name;
                        if (StringCommonPrefixLength(input, name) == input.length) {
                            if (!foundOne) {
                                commonPart = name;
                                foundOne = true;
                            }

                            uint commonLen = StringCommonPrefixLength(commonPart, name);
                            commonPart = commonPart.substr(0, commonLen);
                        }
                    }

                    if (commonPart.length > input.length) {
                        Text = commonPart + Text.substr(whitespace);
                        Select(commonPart.length, 0);
                    }
                }
            } else {
                FieldWithHistory::KeyDown(key);
            }
        }
    }
}
