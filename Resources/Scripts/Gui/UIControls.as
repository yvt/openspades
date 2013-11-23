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
			bool Pressed = false;
			bool Hover = false;
			bool Toggled = false;
			
			bool IsToggleButton = false;
			
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
		
		class FieldBase: UIElement {
			bool Dragging = false;
			EventHandler@ Changed;
			string Text;
			int MarkPosition = 0;
			int CursorPosition = 0;
			
			Vector4 TextColor = Vector4(1.f, 1.f, 1.f, 1.f);
			Vector4 HighlightColor = Vector4(1.f, 1.f, 1.f, 0.3f);
			
			Vector2 TextOrigin = Vector2(0.f, 0.f);
			float TextScale = 1.f;
			
			FieldBase(UIManager@ manager) {
				super(manager);
				IsMouseInteractive = true;
				AcceptsFocus = true;
				@this.Cursor = Cursor(Manager, manager.Renderer.RegisterImage("Gfx/IBeam.png"), Vector2(16.f, 16.f));
			}
			
			int SelectionStart {
				get final { return Min(MarkPosition, CursorPosition); }
				set {
					Select(value, SelectionEnd - value);
				}
			}
			
			int SelectionEnd {
				get final {
					return Max(MarkPosition, CursorPosition);
				}
				set {
					Select(SelectionStart, value - SelectionStart);
				}
			}
			
			int SelectionLength {
				get final {
					return SelectionEnd - SelectionStart;
				}
				set {
					Select(SelectionStart, value);
				}
			}
			
			string SelectedText {
				get final {
					return Text.substr(SelectionStart, SelectionLength);
				}
				set {
					Text = Text.substr(0, SelectionStart) + value + Text.substr(SelectionEnd);
					SelectionLength = value.length;
				}
			}
			
			private int PointToCharIndex(float x) {
				x -= TextOrigin.x;
				if(x < 0.f) return 0;
				x /= TextScale;
				string text = Text;
				int len = text.length;
				float lastWidth = 0.f;
				Font@ font = this.Font;
				// FIXME: use binary search for better performance?
				// FIXME: support multi-byte charset
				for(int i = 1; i <= len; i++) {
					float width = font.Measure(text.substr(0, i)).x;
					if(width > x) {
						if(x < (lastWidth + width) * 0.5f) {
							return i - 1;
						} else {
							return i;
						}
					}
					lastWidth = width;
				}
				return len;
			}
			int PointToCharIndex(Vector2 pt) {
				return PointToCharIndex(pt.x);
			}
			
			int ClampCursorPosition(int pos) {
				return Clamp(pos, 0, Text.length);
			}
			
			void Select(int start, int length = 0) {
				MarkPosition = ClampCursorPosition(start);
				CursorPosition = ClampCursorPosition(start + length);
			}
			
			void SelectAll() {
				Select(0, Text.length);
			}
			
			void BackSpace() {
				if(SelectionLength > 0) {
					SelectedText = "";
				} else {
					Select(SelectionStart - 1, 1);
					SelectedText = "";
				}
			}
			
			void Insert(string text) {
				string oldText = SelectedText;
				SelectedText = text;
				
				// if text overflows, deny the insertion
				if(!FitsInBox(Text)) {
					SelectedText = oldText;
					return;
				}
				
				Select(SelectionEnd);
			}
			
			void KeyDown(string key) {
				if(key == "BackSpace") {
					BackSpace();
				}else if(key == "Left") {
					if(Manager.IsShiftPressed) {
						CursorPosition = ClampCursorPosition(CursorPosition - 1);
					}else {
						if(SelectionLength == 0) {
							// FIXME: support multi-byte charset
							Select(CursorPosition - 1);
						} else {
							Select(SelectionStart);
						}
					}
					return;
				}else if(key == "Right") {
					if(Manager.IsShiftPressed) {
						CursorPosition = ClampCursorPosition(CursorPosition + 1);
					}else {
						if(SelectionLength == 0) {
							// FIXME: support multi-byte charset
							Select(CursorPosition + 1);
						} else {
							Select(SelectionEnd);
						}
					}
					return;
				}
				if(manager.IsControlPressed) {
					if(key == "a") {
						SelectAll();
						return;
					}
				}
				manager.ProcessHotKey(key);
			}
			void KeyUp(string key) {
			}
			
			void KeyPress(string text) {
				if(!manager.IsControlPressed) {
					Insert(text);
				}
			}
			void MouseDown(MouseButton button, Vector2 clientPosition) {
				if(button != spades::ui::MouseButton::LeftMouseButton) {
					return;
				}
				Dragging = true; 
				if(Manager.IsShiftPressed) {
					MouseMove(clientPosition);
				} else {
					Select(PointToCharIndex(clientPosition));
				}
			}
			void MouseMove(Vector2 clientPosition) {
				if(Dragging) {
					CursorPosition = PointToCharIndex(clientPosition);
				}
			}
			void MouseUp(MouseButton button, Vector2 clientPosition) {
				if(button != spades::ui::MouseButton::LeftMouseButton) {
					return;
				}
				Dragging = false;
			}
			
			bool FitsInBox(string text) {
				return Font.Measure(text).x * TextScale < Size.x - TextOrigin.x;
			}
			
			void DrawHighlight(float x, float y, float w, float h) {
				Renderer@ renderer = Manager.Renderer;
				renderer.Color = Vector4(1.f, 1.f, 1.f, 0.2f);
				
				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.DrawImage(img, AABB2(x, y, w, h));
			}
			
			void DrawBeam(float x, float y, float h) {
				Renderer@ renderer = Manager.Renderer;
				float pulse = sin(Manager.Time * 5.f);
				pulse = abs(pulse);
				renderer.Color = Vector4(1.f, 1.f, 1.f, pulse);
				
				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.DrawImage(img, AABB2(x - 1.f, y, 2, h));
			}
			
			void Render() {
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				Font@ font = this.Font;
				Vector2 textPos = TextOrigin + pos;
				string text = Text;
				
				font.Draw(text, textPos, TextScale, TextColor);
				
				if(IsFocused){
					float fontHeight = font.Measure("A").y;
					
					// draw selection
					int start = SelectionStart;
					int end = SelectionEnd;
					if(end == start) {
						float x = font.Measure(text.substr(0, start)).x;
						DrawBeam(x + textPos.x, textPos.y, fontHeight);
					} else {
						float x1 = font.Measure(text.substr(0, start)).x;
						float x2 = font.Measure(text.substr(0, end)).x;
						DrawHighlight(textPos.x + x1, textPos.y, x2 - x1, fontHeight);
					}
				}
			}
		}
		
		class Field: FieldBase {
			Field(UIManager@ manager) {
				super(manager);
				TextOrigin = Vector2(2.f, 2.f);
			}
			void Render() {
				// render background
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.Color = Vector4(0.f, 0.f, 0.f, IsFocused ? 0.3f : 0.1f);
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
				
				FieldBase::Render();
			}
		}
	}
}