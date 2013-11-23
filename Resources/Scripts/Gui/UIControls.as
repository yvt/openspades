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
		funcdef void EventHandler(UIElement@ sender);
		
		class ButtonBase: UIElement {
			bool Pressed;
			bool Hover;
			bool Toggled;
			
			bool IsToggleButton;
			
			EventHandler@ Activated;
			string Caption;
			string ActivateHotKey;
			
			ButtonBase(UIManager@ manager) {
				super(manager);
				IsMouseInteractive = true;
			}
			
			void PlayMouseEnterSound() {
				Manager.PlaySound("Sounds/Feedback/Limbo/Hover.wav");
			}
			
			void PlayActivateSound() {
				Manager.PlaySound("Sounds/Feedback/Limbo/Select.wav");
			}
			
			void OnActivated() {
				if(Activated !is null) {
					Activated(this);
				}
			}
			
			void MouseDown(MouseButton button, Vector2 clientPosition) {
				if(button != spades::ui::MouseButton::LeftMouseButton) {
					return;
				}
				Pressed = true;
				PlayActivateSound();
			}
			void MouseMove(Vector2 clientPosition) {
			}
			void MouseUp(MouseButton button, Vector2 clientPosition) {
				if(button != spades::ui::MouseButton::LeftMouseButton) {
					return;
				}
				if(Pressed) {
					Pressed = false;
					if(Hover) {
						if(IsToggleButton) {
							Toggled = not Toggled;
						}
						OnActivated();
					}
				}
			}
			void MouseEnter() {
				Hover = true;
				if(not Pressed) {
					PlayMouseEnterSound();
				}
			}
			void MouseLeave() {
				Hover = false;
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
		
		class Button: ButtonBase {
			private Image@ image;
			
			Button(UIManager@ manager) {
				super(manager);
				
				Renderer@ renderer = Manager.Renderer;
				@image = renderer.RegisterImage("Gfx/Limbo/MenuItem.tga");
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
				renderer.Color = color;
				
				DrawSliceImage(renderer, image, pos.x, pos.y, size.x, size.y, 12.f);
				
				Font@ font = this.Font;
				string text = this.Caption;
				Vector2 txtSize = font.Measure(text);
				Vector2 txtPos;
				txtPos = pos + (size - txtSize) * 0.5f;
				
				font.DrawShadow(text, txtPos, 1.f, 
					Vector4(1.f, 1.f, 1.f, 1.f), Vector4(0.f, 0.f, 0.f, 0.4f));
			}
			
		}
	}
}