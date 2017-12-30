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

#include "ScrollBar.as"

namespace spades {
    namespace ui {

        class SliderKnob: UIElement {
            private Slider@ slider;
            private bool dragging = false;
            private double startValue;
            private float startCursorPos;
            private bool hover = false;

            SliderKnob(Slider@ slider) {
                super(slider.Manager);
                @this.slider = slider;
                IsMouseInteractive = true;
            }

            private float GetCursorPos(Vector2 pos) {
                return pos.x + Position.x;
            }

            void MouseDown(MouseButton button, Vector2 clientPosition) {
                if(button != spades::ui::MouseButton::LeftMouseButton) {
                    return;
                }
                dragging = true;
                startValue = slider.Value;
                startCursorPos = GetCursorPos(clientPosition);
            }
            void MouseMove(Vector2 clientPosition) {
                if(dragging) {
                    double val = startValue;
                    float delta = GetCursorPos(clientPosition) - startCursorPos;
                    val += delta * (slider.MaxValue - slider.MinValue) /
                        double(slider.TrackBarMovementRange);
                    slider.ScrollTo(val);
                }
            }
            void MouseUp(MouseButton button, Vector2 clientPosition) {
                if(button != spades::ui::MouseButton::LeftMouseButton) {
                    return;
                }
                dragging = false;
            }
            void MouseEnter() {
                hover = true;
                UIElement::MouseEnter();
            }
            void MouseLeave() {
                hover = false;
                UIElement::MouseLeave();
            }

            void Render() {
                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");

                if (hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.5f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.3f);
                }
                renderer.DrawImage(img,
                    AABB2(pos.x + size.x * 0.5f - 3.f, pos.y,
                    6.f, size.y));

                renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.6f);
                renderer.DrawImage(img,
                    AABB2(pos.x + size.x * 0.5f - 2.f, pos.y + 1.f,
                    4.f, size.y - 2.f));
            }
        }

        class Slider: ScrollBarBase {

            private SliderKnob@ knob;
            private ScrollBarFill@ fill1;
            private ScrollBarFill@ fill2;

            Slider(UIManager@ manager) {
                super(manager);

                @knob = SliderKnob(this);
                AddChild(knob);

                @fill1 = ScrollBarFill(this, false);
                @fill1.Activated = EventHandler(this.LargeDown);
                AddChild(fill1);
                @fill2 = ScrollBarFill(this, true);
                @fill2.Activated = EventHandler(this.LargeUp);
                AddChild(fill2);

            }

            private void LargeDown(UIElement@ e) {
                ScrollBy(-LargeChange);
            }
            private void LargeUp(UIElement@ e) {
                ScrollBy(LargeChange);
            }/*
            private void SmallDown(UIElement@ e) {
                ScrollBy(-SmallChange);
            }
            private void SmallUp(UIElement@ e) {
                ScrollBy(SmallChange);
            }*/

            void OnChanged() {
                Layout();
                ScrollBarBase::OnChanged();
                Layout();
            }

            void Layout() {
                Vector2 size = Size;
                float tPos = TrackBarPosition;
                float tLen = TrackBarLength;
                fill1.Bounds = AABB2(0.f, 0.f, tPos, size.y);
                fill2.Bounds = AABB2(tPos + tLen, 0.f, size.x - tPos - tLen, size.y);
                knob.Bounds = AABB2(tPos, 0.f, tLen, size.y);
            }

            void OnResized() {
                Layout();
                UIElement::OnResized();
            }

            float Length {
                get {
                    if(Orientation == spades::ui::ScrollBarOrientation::Horizontal) {
                        return Size.x;
                    } else {
                        return Size.y;
                    }
                }
            }

            float TrackBarAreaLength {
                get {
                    return Length;
                }
            }

            float TrackBarLength {
                get {
                    return 16.f;
                }
            }

            float TrackBarMovementRange {
                get {
                    return TrackBarAreaLength - TrackBarLength;
                }
            }

            float TrackBarPosition {
                get {
                    return float((Value - MinValue) / (MaxValue - MinValue) * TrackBarMovementRange);
                }
            }

            void Render() {
                Layout();

                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");

                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.1f);
                renderer.DrawImage(img,
                    AABB2(pos.x, pos.y + size.y * 0.5f - 3.f,
                    size.x, 6.f));

                renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.2f);
                renderer.DrawImage(img,
                    AABB2(pos.x + 1.f, pos.y + size.y * 0.5f - 2.f,
                    size.x - 2.f, 4.f));

                ScrollBarBase::Render();
            }
        }

    }
}
