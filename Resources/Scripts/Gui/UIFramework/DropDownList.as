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

namespace spades {
    namespace ui {

        class DropDownViewItem : spades::ui::Button {
            int index;
            DropDownViewItem(spades::ui::UIManager @manager) { super(manager); }
            void Render() {
                Renderer @renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image @img = renderer.RegisterImage("Gfx/White.tga");
                if (Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.12f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.00f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
                if (Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.00f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
                renderer.DrawImage(img, AABB2(pos.x + size.x - 1.f, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y + size.y - 1.f, size.x, 1.f));
                Vector2 txtSize = Font.Measure(Caption);
                Font.DrawShadow(Caption,
                                pos + (size - txtSize) * Vector2(0.f, 0.5f) + Vector2(4.f, 0.f),
                                1.f, Vector4(1, 1, 1, 1), Vector4(0, 0, 0, 0.4f));
            }
        }

        class DropDownViewModel : spades::ui::ListViewModel {
            spades::ui::UIManager @manager;
            string[] @list;

            DropDownListHandler @Handler;

            DropDownViewModel(spades::ui::UIManager @manager, string[] @list) {
                @this.manager = manager;
                @this.list = list;
            }
            int NumRows {
                get { return int(list.length); }
            }
            private void ItemClicked(spades::ui::UIElement @sender) {
                Handler(cast<DropDownViewItem>(sender).index);
            }
            spades::ui::UIElement @CreateElement(int row) {
                DropDownViewItem i(manager);
                i.Caption = list[row];
                i.index = row;
                @i.Activated = spades::ui::EventHandler(this.ItemClicked);
                return i;
            }
            void RecycleElement(spades::ui::UIElement @elem) {}
        }

        class DropDownBackground : UIElement {
            DropDownView @view;
            UIElement @oldRoot;
            DropDownBackground(UIManager @manager, DropDownView @view) {
                super(manager);
                IsMouseInteractive = true;
                @this.view = view;
            }
            void Destroy() {
                @Parent = null;
                if (oldRoot !is null) {
                    oldRoot.Enable = true;
                }
            }
            void MouseDown(MouseButton button, Vector2 clientPosition) {
                view.Destroy();
                view.handler(-1);
            }
            void Render() {
                Renderer @renderer = Manager.Renderer;
                Image @img = renderer.RegisterImage("Gfx/White.tga");
                renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.5f);
                renderer.DrawImage(img, ScreenBounds);
                UIElement::Render();
            }
        }

        class DropDownView : ListView {

            DropDownListHandler @handler;
            DropDownBackground @bg;

            DropDownView(UIManager @manager, DropDownListHandler @handler, string[] @items) {
                super(manager);
                @this.handler = handler;

                DropDownViewModel model(manager, items);
                @model.Handler = DropDownListHandler(this.ItemActivated);
                @this.Model = model;
            }

            void Destroy() { bg.Destroy(); }

            private void ItemActivated(int index) {
                Destroy();
                handler(index);
            }
        }

        funcdef void DropDownListHandler(int index);

        void ShowDropDownList(UIManager @manager, float x, float y, float width, string[] @items,
                              DropDownListHandler @handler) {

            DropDownView view(manager, handler, items);
            DropDownBackground bg(manager, view);
            @view.bg = bg;

            UIElement @root = manager.RootElement;
            Vector2 size = root.Size;

            float maxHeight = size.y - y;
            float height = 24.f * float(items.length);
            if (height > maxHeight)
                height = maxHeight;

            bg.Bounds = AABB2(0.f, 0.f, size.x, size.y);
            view.Bounds = AABB2(x, y, width, height);

            UIElement @[] @roots = root.GetChildren();
            /*for(uint i = 0; i < roots.length; i++) {
                    UIElement@ r = roots[i];
                    if(r.Enable) {
                            @bg.oldRoot = r;
                            r.Enable = false;
                            break;
                    }
            }*/

            root.AddChild(bg);
            bg.AddChild(view);
        }
        void ShowDropDownList(UIElement @e, string[] @items, DropDownListHandler @handler) {
            AABB2 b = e.ScreenBounds;
            ShowDropDownList(e.Manager, b.min.x, b.max.y, b.max.x - b.min.x, items, handler);
        }
    }
}
