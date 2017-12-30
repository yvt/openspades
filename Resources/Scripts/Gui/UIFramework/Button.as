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

#include "UIFramework.as"

namespace spades {
    namespace ui {

        class ButtonBase: UIElement {
            bool Pressed = false;
            bool Hover = false;
            bool Toggled = false;

            bool Toggle = false;
            bool Repeat = false;
            bool ActivateOnMouseDown = false;

            EventHandler@ Activated;
            EventHandler@ DoubleClicked;
            EventHandler@ RightClicked;
            string Caption;
            string ActivateHotKey;

            private Timer@ repeatTimer;

            // for double click detection
            private float lastActivate = -1.f;
            private Vector2 lastActivatePosition = Vector2();

            ButtonBase(UIManager@ manager) {
                super(manager);
                IsMouseInteractive = true;
                @repeatTimer = Timer(Manager);
                @repeatTimer.Tick = TimerTickEventHandler(this.RepeatTimerFired);
            }

            void PlayMouseEnterSound() {
                Manager.PlaySound("Sounds/Feedback/Limbo/Hover.opus");
            }

            void PlayActivateSound() {
                Manager.PlaySound("Sounds/Feedback/Limbo/Select.opus");
            }

            void OnActivated() {
                if(Activated !is null) {
                    Activated(this);
                }
            }

            void OnDoubleClicked() {
                if(DoubleClicked !is null) {
                    DoubleClicked(this);
                }
            }

            void OnRightClicked() {
                if(RightClicked !is null) {
                    RightClicked(this);
                }
            }

            private void RepeatTimerFired(Timer@ timer) {
                OnActivated();
                timer.Interval = 0.1f;
            }

            void MouseDown(MouseButton button, Vector2 clientPosition) {
                if(button != spades::ui::MouseButton::LeftMouseButton &&
                   button != spades::ui::MouseButton::RightMouseButton) {
                    return;
                }

                PlayActivateSound();
                if (button == spades::ui::MouseButton::RightMouseButton) {
                    OnRightClicked();
                    return;
                }

                Pressed = true;
                Hover = true;

                if(Repeat or ActivateOnMouseDown) {
                    OnActivated();
                    if(Repeat) {
                        repeatTimer.Interval = 0.3f;
                        repeatTimer.Start();
                    }
                }
            }
            void MouseMove(Vector2 clientPosition) {
                if(Pressed) {
                    bool newHover = AABB2(Vector2(0.f, 0.f), Size).Contains(clientPosition);
                    if(newHover != Hover) {
                        if(Repeat) {
                            if(newHover) {
                                OnActivated();
                                repeatTimer.Interval = 0.3f;
                                repeatTimer.Start();
                            } else {
                                repeatTimer.Stop();
                            }
                        }
                        Hover = newHover;
                    }
                }
            }
            void MouseUp(MouseButton button, Vector2 clientPosition) {
                if(button != spades::ui::MouseButton::LeftMouseButton &&
                   button != spades::ui::MouseButton::RightMouseButton) {
                    return;
                }
                if(Pressed) {
                    Pressed = false;
                    if(Hover and not (Repeat or ActivateOnMouseDown)) {
                        if(Toggle) {
                            Toggled = not Toggled;
                        }
                        OnActivated();
                        if (Manager.Time < lastActivate + 0.35 &&
                            (clientPosition - lastActivatePosition).ManhattanLength < 10.0f) {
                            OnDoubleClicked();
                        }
                        lastActivate = Manager.Time;
                        lastActivatePosition = clientPosition;
                    }

                    if(Repeat and Hover){
                        repeatTimer.Stop();
                    }
                }
            }
            void MouseEnter() {
                Hover = true;
                if(not Pressed) {
                    PlayMouseEnterSound();
                }
                UIElement::MouseEnter();
            }
            void MouseLeave() {
                Hover = false;
                UIElement::MouseLeave();
            }

            void KeyDown(string key) {
                if(key == " ") {
                    OnActivated();
                }
                UIElement::KeyDown(key);
            }
            void KeyUp(string key) {
                UIElement::KeyUp(key);
            }

            void HotKey(string key) {
                if(key == ActivateHotKey) {
                    OnActivated();
                }
            }

        }

        class SimpleButton: spades::ui::Button {
            SimpleButton(spades::ui::UIManager@ manager){
                super(manager);
            }
            void Render() {
                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");
                if((Pressed && Hover) || Toggled) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.12f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
                if((Pressed && Hover) || Toggled) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.1f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.03f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
                renderer.DrawImage(img, AABB2(pos.x+size.x-1.f, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y+size.y-1.f, size.x, 1.f));
                Vector2 txtSize = Font.Measure(Caption);
                float margin = 4.f;
                Font.DrawShadow(Caption, pos + Vector2(margin, margin) +
                    (size - txtSize - Vector2(margin * 2.f, margin * 2.f)) * Alignment,
                    1.f, Vector4(1,1,1,1), Vector4(0,0,0,0.4f));
            }
        }


        class CheckBox: spades::ui::Button {
            CheckBox(spades::ui::UIManager@ manager){
                super(manager);
                this.Toggle = true;
            }
            void Render() {
                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");
                if(Pressed && Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.12f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.00f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
                Vector2 txtSize = Font.Measure(Caption);
                Font.DrawShadow(Caption, pos + (size - txtSize) * Vector2(0.f, 0.5f) + Vector2(16.f, 0.f),
                    1.f, Vector4(1,1,1,1), Vector4(0,0,0,0.2f));

                @img = renderer.RegisterImage("Gfx/UI/CheckBox.png");

                renderer.ColorNP = Vector4(1.f, 1.f, 1.f, Toggled ? .9f : 0.6f);
                renderer.DrawImage(img, AABB2(pos.x, pos.y + (size.y - 16.f) * 0.5f, 16.f, 16.f),
                    AABB2(Toggled ? 16.f : 0.f, 0.f, 16.f, 16.f));

            }
        }


        class RadioButton: spades::ui::Button {
            string GroupName;

            RadioButton(spades::ui::UIManager@ manager){
                super(manager);
                this.Toggle = true;
            }
            void Check() {
                this.Toggled = true;

                // uncheck others
                if(GroupName.length > 0) {
                    UIElement@[]@ children = this.Parent.GetChildren();
                    for(uint i = 0, count = children.length; i < children.length; i++) {
                        RadioButton@ btn = cast<RadioButton>(children[i]);
                        if(btn is this) continue;
                        if(btn !is null) {
                            if(GroupName == btn.GroupName) {
                                btn.Toggled = false;
                            }
                        }
                    }
                }
            }
            void OnActivated() {
                Check();

                Button::OnActivated();
            }
            void Render() {
                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;
                Image@ img = renderer.RegisterImage("Gfx/White.tga");
                if(!this.Enable) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                } else if((Pressed && Hover) || Toggled) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.12f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
                if(!this.Enable) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.03f);
                } if((Pressed && Hover) || Toggled) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.1f);
                } else if(Hover) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.07f);
                } else {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.03f);
                }
                renderer.DrawImage(img, AABB2(pos.x, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
                renderer.DrawImage(img, AABB2(pos.x+size.x-1.f, pos.y, 1.f, size.y));
                renderer.DrawImage(img, AABB2(pos.x, pos.y+size.y-1.f, size.x, 1.f));
                Vector2 txtSize = Font.Measure(Caption);
                Font.DrawShadow(Caption, pos + (size - txtSize) * 0.5f + Vector2(8.f, 0.f), 1.f,
                    Vector4(1,1,1,this.Enable ? 1.f : 0.4f), Vector4(0,0,0,0.4f));

                if(Toggled) {
                    renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.6f);
                    renderer.DrawImage(img, AABB2(pos.x + 4.f, pos.y + (size.y - 8.f) * 0.5f, 8.f, 8.f));
                }


            }
        }

        class Button: ButtonBase {
            private Image@ image;
            Vector2 Alignment = Vector2(0.5f, 0.5f);

            Button(UIManager@ manager) {
                super(manager);

                Renderer@ renderer = Manager.Renderer;
                @image = renderer.RegisterImage("Gfx/UI/Button.png");
            }

            void Render() {
                Renderer@ renderer = Manager.Renderer;
                Vector2 pos = ScreenPosition;
                Vector2 size = Size;

                Vector4 color = Vector4(0.2f, 0.2f, 0.2f, 0.5f);
                if(Toggled or (Pressed and Hover)) {
                    color = Vector4(0.7f, 0.7f, 0.7f, 0.9f);
                }else if(Hover) {
                    color = Vector4(0.4f, 0.4f, 0.4f, 0.7f);
                }
                if(!IsEnabled) {
                    color.w *= 0.5f;
                }
                renderer.ColorNP = color;

                DrawSliceImage(renderer, image, pos.x, pos.y, size.x, size.y, 12.f);

                Font@ font = this.Font;
                string text = this.Caption;
                Vector2 txtSize = font.Measure(text);
                Vector2 txtPos;
                pos += Vector2(8.f, 8.f);
                size -= Vector2(16.f, 16.f);
                txtPos = pos + (size - txtSize) * Alignment;

                if(IsEnabled){
                    font.DrawShadow(text, txtPos, 1.f,
                        Vector4(1.f, 1.f, 1.f, 1.f), Vector4(0.f, 0.f, 0.f, 0.4f));
                }else{
                    font.DrawShadow(text, txtPos, 1.f,
                        Vector4(1.f, 1.f, 1.f, 0.5f), Vector4(0.f, 0.f, 0.f, 0.1f));
                }
            }

        }

    }
}
