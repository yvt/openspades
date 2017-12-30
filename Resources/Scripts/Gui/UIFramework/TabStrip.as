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

#include "Button.as"

namespace spades {
    namespace ui {

        class SimpleTabStripItem: ButtonBase {
            UIElement@ linkedElement;

            SimpleTabStripItem(UIManager@ manager, UIElement@ linkedElement) {
                super(manager);
                @this.linkedElement = linkedElement;
            }

            void MouseDown(MouseButton button, Vector2 clientPosition) {
                PlayActivateSound();
                OnActivated();
            }

            void Render() {
                this.Toggled = linkedElement.Visible;

                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Vector4 textColor(0.9f, 0.9f, 0.9f, 1.0f);
                Image@ img = renderer.RegisterImage("Gfx/White.tga");
                if(Toggled) {
                    renderer.ColorNP = Vector4(0.9f, 0.9f, 0.9f, 1.0f);
                    textColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.3f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.0f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));

                Vector2 txtSize = Font.Measure(Caption);
                Font.Draw(Caption, pos + (size - txtSize) * 0.5f, 1.f, textColor);
            }
        }

        class SimpleTabStrip: UIElement {
            private float nextX = 0.f;

            EventHandler@ Changed;

            SimpleTabStrip(UIManager@ manager) {
                super(manager);
            }

            private void OnChanged() {
                if(Changed !is null) {
                    Changed(this);
                }
            }

            private void OnItemActivated(UIElement@ sender) {
                SimpleTabStripItem@ item = cast<SimpleTabStripItem>(sender);
                UIElement@ linked = item.linkedElement;
                UIElement@[]@ children = this.GetChildren();
                for(uint i = 0, count = children.length; i < count; i++) {
                    SimpleTabStripItem@ otherItem = cast<SimpleTabStripItem>(children[i]);
                    otherItem.linkedElement.Visible = (otherItem.linkedElement is linked);
                }
                OnChanged();
            }

            void AddItem(string title, UIElement@ linkedElement) {
                SimpleTabStripItem item(this.Manager, linkedElement);
                item.Caption = title;
                float w = this.Font.Measure(title).x + 18.f;
                item.Bounds = AABB2(nextX, 0.f, w, 24.f);
                nextX += w + 4.f;

                @item.Activated = EventHandler(this.OnItemActivated);

                this.AddChild(item);

            }

            void Render() {
                UIElement::Render();

                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");
                renderer.ColorNP = Vector4(0.9f, 0.9f, 0.9f, 1.0f);
                renderer.DrawImage(img, AABB2(pos.x, pos.y + 24.f, size.x, 1.f));
            }

        }

    }
}
