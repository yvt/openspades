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

#include "Label.as"

namespace spades {
    namespace ui {
        class TextViewerSelectionState {
            UIElement @FocusElement;
            int MarkPosition = 0;
            int CursorPosition = 0;

            int SelectionStart {
                get final { return Min(MarkPosition, CursorPosition); }
            }

            int SelectionEnd {
                get final { return Max(MarkPosition, CursorPosition); }
            }
        }

        class TextViewerItemUI : UIElement {
            private string text;
            private Vector4 textColor;
            private int index;

            private TextViewerSelectionState @selection;

            TextViewerItemUI(UIManager @manager, TextViewerItem @item,
                             TextViewerSelectionState @selection) {
                super(manager);

                text = item.Text;
                textColor = item.Color;
                index = item.Index;
                @this.selection = selection;
            }

            void DrawHighlight(float x, float y, float w, float h) {
                Renderer @renderer = Manager.Renderer;
                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);

                Image @img = renderer.RegisterImage("Gfx/White.tga");
                renderer.DrawImage(img, AABB2(x, y, w, h));
            }

            void Render() {
                Renderer @renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                float textScale = 1.0f;
                Font @font = this.Font;

                if (text.length > 0) {
                    Vector2 txtSize = font.Measure(text) * textScale;
                    Vector2 txtPos;
                    txtPos = pos + (size - txtSize) * Vector2(0.0f, 0.0f);

                    font.Draw(text, txtPos, textScale, textColor);
                }

                if (selection.FocusElement.IsFocused) {
                    // Draw selection
                    int start = selection.SelectionStart - index;
                    int end = selection.SelectionEnd - index;
                    if (start < 0) {
                        start = 0;
                    }
                    if (end > int(text.length) + 1) {
                        end = int(text.length) + 1;
                    }
                    if (end > start) {
                        float x1 = font.Measure(text.substr(0, start)).x;
                        float x2 = font.Measure(text.substr(0, end)).x;

                        if (end == int(text.length) + 1) {
                            x2 = size.x;
                        }

                        DrawHighlight(pos.x + x1, pos.y, x2 - x1, size.y);
                    }
                }
            }
        }

        class TextViewerItem {
            string Text;
            Vector4 Color;
            int Index;

            TextViewerItem(string text, Vector4 color, int index) {
                Text = text;
                Color = color;
                Index = index;
            }
        }

        class TextViewerModel : ListViewModel {
            UIManager @manager;
            TextViewerItem @[] lines = {};
            Font @font;
            float width;
            TextViewerSelectionState @selection;
            int contentStart;
            int contentEnd;

            void AddLine(string text, Vector4 color) {
                int startPos = 0;
                if (font.Measure(text).x <= width) {
                    lines.insertLast(TextViewerItem(text, color, contentEnd));
                    contentEnd += text.length + 1;
                    return;
                }

                int pos = 0;
                int len = int(text.length);
                bool charMode = false;
                while (startPos < len) {
                    int nextPos = pos + 1;
                    if (charMode) {
                        // skip to the next UTF-8 character boundary
                        while (nextPos < len && ((text[nextPos] & 0x80) != 0) &&
                               ((text[nextPos] & 0xc0) != 0xc0))
                            nextPos++;
                    } else {
                        while (nextPos < len && text[nextPos] != 0x20)
                            nextPos++;
                    }
                    if (font.Measure(text.substr(startPos, nextPos - startPos)).x > width) {
                        if (pos == startPos) {
                            if (charMode) {
                                pos = nextPos;
                            } else {
                                charMode = true;
                            }
                            continue;
                        } else {
                            lines.insertLast(TextViewerItem(text.substr(startPos, pos - startPos),
                                                            color, contentEnd));
                            contentEnd += pos - startPos;
                            startPos = pos;
                            while (startPos < len && text[startPos] == 0x20) {
                                startPos++;
                            }
                            pos = startPos;
                            charMode = false;
                            continue;
                        }
                    } else {
                        pos = nextPos;
                        if (nextPos >= len) {
                            lines.insertLast(TextViewerItem(
                                text.substr(startPos, nextPos - startPos), color, contentEnd));
                            contentEnd += nextPos - startPos + 1;
                            break;
                        }
                    }
                }
            }

            /**
             * Remove the first line from the model.
             *
             * `ListViewModel` doesn't support removing items from other places
             * than the end of the list. Therefore, after calling this,
             * `ListViewBase.Model` must be reassigned to recreate all elements
             * in view.
             */
            void RemoveFirstLines(uint numLines) {
                int removedLength;
                if (lines.length > numLines) {
                    removedLength = lines[numLines].Index - contentStart;
                } else {
                    removedLength = contentEnd - contentStart;
                }

                lines.removeRange(0, numLines);
                contentStart += removedLength;

                selection.MarkPosition = Max(selection.MarkPosition, contentStart);
                selection.CursorPosition = Max(selection.CursorPosition, contentStart);
            }

            TextViewerModel(UIManager @manager, string text, Font @font, float width,
                            TextViewerSelectionState @selection) {
                @this.manager = manager;
                @this.font = font;
                this.width = width;
                @this.selection = selection;
                string[] @lines = text.split("\n");
                for (uint i = 0; i < lines.length; i++)
                    AddLine(lines[i], Vector4(1.f, 1.f, 1.f, 1.f));
            }

            int NumRows {
                get { return int(lines.length); }
            }

            UIElement @CreateElement(int row) {
                return TextViewerItemUI(manager, lines[row], selection);
            }

            void RecycleElement(UIElement @elem) {}
        }

        class TextViewer : ListViewBase {
            private string text;
            private TextViewerModel @textmodel;
            private TextViewerSelectionState selection;
            private bool dragging = false;

            /**
             * The maximum number of lines. This affects the behavior of the
             * `AddLine` method. `0` means unlimited.
             */
            int MaxNumLines = 0;

            TextViewer(UIManager @manager) {
                super(manager);

                @selection.FocusElement = this;
                AcceptsFocus = true;
                IsMouseInteractive = true;
                @this.Cursor
                = Cursor(Manager, manager.Renderer.RegisterImage("Gfx/UI/IBeam.png"),
                         Vector2(16.f, 16.f));
            }

            /**
             * Sets the displayed text. Ensure TextViewer.Font is not null before
             * setting this proeprty.
             */
            string Text {
                get final { return text; }
                set {
                    text = value;
                    @textmodel = TextViewerModel(Manager, text, Font, ItemWidth, selection);
                    @Model = textmodel;
                }
            }

            private int PointToCharIndex(Vector2 clientPosition) {
                int line = int(clientPosition.y / RowHeight) + TopRowIndex;
                if (line < 0) {
                    return textmodel.contentStart;
                }
                if (line >= int(textmodel.lines.length)) {
                    return textmodel.contentEnd;
                }

                float x = clientPosition.x;
                string text = textmodel.lines[line].Text;
                int lineStartIndex = textmodel.lines[line].Index;
                if (x < 0.f) {
                    return lineStartIndex;
                }
                int len = text.length;
                float lastWidth = 0.f;
                Font @font = this.Font;
                // FIXME: use binary search for better performance?
                int idx = 0;
                for (int i = 1; i <= len; i++) {
                    int lastIdx = idx;
                    idx = GetByteIndexForString(text, 1, idx);
                    float width = font.Measure(text.substr(0, idx)).x;
                    if (width > x) {
                        if (x < (lastWidth + width) * 0.5f) {
                            return lastIdx + lineStartIndex;
                        } else {
                            return idx + lineStartIndex;
                        }
                    }
                    lastWidth = width;
                    if (idx >= len) {
                        return len + lineStartIndex;
                    }
                }
                return len + lineStartIndex;
            }

            void MouseDown(MouseButton button, Vector2 clientPosition) {
                if (button != spades::ui::MouseButton::LeftMouseButton) {
                    return;
                }
                dragging = true;
                if (Manager.IsShiftPressed) {
                    MouseMove(clientPosition);
                } else {
                    selection.MarkPosition = selection.CursorPosition =
                        PointToCharIndex(clientPosition);
                }
            }

            void MouseMove(Vector2 clientPosition) {
                if (dragging) {
                    selection.CursorPosition = PointToCharIndex(clientPosition);
                }
            }

            void MouseUp(MouseButton button, Vector2 clientPosition) {
                if (button != spades::ui::MouseButton::LeftMouseButton) {
                    return;
                }
                dragging = false;
            }

            void KeyDown(string key) {
                if (Manager.IsControlPressed or Manager.IsMetaPressed /* for OSX; Cmd + [a-z] */) {
                    if (key == "C" && this.selection.SelectionEnd > this.selection.SelectionStart) {
                        Manager.Copy(this.SelectedText);
                        return;
                    } else if (key == "A") {
                        this.selection.MarkPosition = textmodel.contentStart;
                        this.selection.CursorPosition = textmodel.contentEnd;
                        return;
                    }
                }
                Manager.ProcessHotKey(key);
            }

            string SelectedText {
                get final {
                    string result;
                    int start = this.selection.SelectionStart;
                    int end = this.selection.SelectionEnd;

                    auto @lines = textmodel.lines;

                    for (uint i = 0, count = lines.length; i < count; ++i) {
                        string line = lines[i].Text;
                        int lineStart = lines[i].Index;
                        int lineEnd = lineStart + int(line.length);

                        if (end >= lineStart && start <= lineEnd) {
                            int substrStart = Max(start - lineStart, 0);
                            int substrEnd = Min(end - lineStart, int(line.length));
                            result += line.substr(substrStart, substrEnd - substrStart);
                        }

                        if (i < lines.length - 1 && lineEnd < lines[i + 1].Index) {
                            // Implicit new line
                            if (lineEnd >= start && lineEnd < end) {
                                result += "\n";
                            }
                        }
                    }

                    return result;
                }
            }

            void AddLine(string line, bool autoscroll = false,
                         Vector4 color = Vector4(1.f, 1.f, 1.f, 1.f)) {
                if (textmodel is null) {
                    this.Text = "";
                }
                if (autoscroll) {
                    this.Layout();
                    if (this.scrollBar.Value < this.scrollBar.MaxValue) {
                        autoscroll = false;
                    }
                }
                textmodel.AddLine(line, color);
                if (MaxNumLines > 0 && textmodel.NumRows > MaxNumLines) {
                    textmodel.RemoveFirstLines(textmodel.NumRows - MaxNumLines);
                    @Model = textmodel;
                }
                if (autoscroll) {
                    this.Layout();
                    this.ScrollToEnd();
                }
            }
        }

    }
}
