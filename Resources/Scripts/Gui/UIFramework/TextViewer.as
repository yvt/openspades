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

        class TextViewerModel: ListViewModel {
            UIManager@ manager;
            string[]@ lines = array<string>();
            Vector4[]@ colors = array<spades::Vector4>();
            Font@ font;
            float width;
            void AddLine(string text, Vector4 color) {
                int startPos = 0;
                if(font.Measure(text).x <= width) {
                    lines.insertLast(text);
                    colors.insertLast(color);
                    return;
                }

                int pos = 0;
                int len = int(text.length);
                bool charMode = false;
                while(startPos < len) {
                    int nextPos = pos + 1;
                    if(charMode) {
                        // skip to the next UTF-8 character boundary
                        while(nextPos < len && ((text[nextPos] & 0x80) != 0) &&
                              ((text[nextPos] & 0xc0) != 0xc0))
                            nextPos++;
                    } else {
                        while(nextPos < len && text[nextPos] != 0x20)
                            nextPos++;
                    }
                    if(font.Measure(text.substr(startPos, nextPos - startPos)).x > width) {
                        if(pos == startPos) {
                            if(charMode) {
                                pos = nextPos;
                            }else{
                                charMode = true;
                            }
                            continue;
                        }else{
                            lines.insertLast(text.substr(startPos, pos - startPos));
                            colors.insertLast(color);
                            startPos = pos;
                            while(startPos < len && text[startPos] == 0x20)
                                startPos++;
                            pos = startPos;
                            charMode = false;
                            continue;
                        }
                    }else{
                        pos = nextPos;
                        if(nextPos >= len) {
                            lines.insertLast(text.substr(startPos, nextPos - startPos));
                            colors.insertLast(color);
                            break;
                        }
                    }
                }

            }
            TextViewerModel(UIManager@ manager, string text, Font@ font, float width) {
                @this.manager = manager;
                @this.font = font;
                this.width = width;
                string[]@ lines = text.split("\n");
                for(uint i = 0; i < lines.length; i++)
                    AddLine(lines[i], Vector4(1.f, 1.f, 1.f, 1.f));
            }
            int NumRows { get { return int(lines.length); } }
            UIElement@ CreateElement(int row) {
                Label i(manager);
                i.Text = lines[row];
                i.TextColor = colors[row];
                return i;
            }
            void RecycleElement(UIElement@ elem) {}
        }

        class TextViewer: ListViewBase {
            private string text;
            private TextViewerModel@ textmodel;

            TextViewer(UIManager@ manager) {
                super(manager);
            }

            /** Sets the displayed text. Ensure TextViewer.Font is not null before setting this proeprty. */
            string Text {
                get final { return text; }
                set {
                    text = value;
                    @textmodel = TextViewerModel(Manager, text, Font, ItemWidth);
                    @Model = textmodel;
                }
            }

            void AddLine(string line, bool autoscroll = false, Vector4 color = Vector4(1.f, 1.f, 1.f, 1.f)) {
                if(textmodel is null) {
                    this.Text = "";
                }
                if(autoscroll){
                    this.Layout();
                    if(this.scrollBar.Value < this.scrollBar.MaxValue) {
                        autoscroll = false;
                    }
                }
                textmodel.AddLine(line, color);
                if(autoscroll) {
                    this.Layout();
                    this.ScrollToEnd();
                }
            }
        }

    }
}
