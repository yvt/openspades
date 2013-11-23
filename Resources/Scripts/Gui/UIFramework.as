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
		/** Manages all input/output and rendering of the UI framework. */
		class UIManager {
			private Renderer@ renderer;
			private AudioDevice@ audioDevice;
			
			Vector2 MouseCursorPosition;
			UIElement@ RootElement;
			UIElement@ ActiveElement;
			Cursor@ DefaultCursor;
			
			private UIElement@ mouseCapturedElement;
			private UIElement@ mouseHoverElement;
			
			Renderer@ Renderer {
				get final { return renderer; }
			}
			
			AudioDevice@ AudioDevice {
				get final { return audioDevice; }
			}
			
			UIManager(Renderer@ renderer, AudioDevice@ audioDevice) {
				@this.renderer = renderer;
				@this.audioDevice = audioDevice;
				
				@RootElement = UIElement(this);
				RootElement.Size = Vector2(renderer.ScreenWidth, renderer.ScreenHeight);
				
				@DefaultCursor = Cursor(this, renderer.RegisterImage("Gfx/Limbo/Cursor.tga"), Vector2(16.f, 16.f));
			}
			
			private MouseButton TranslateMouseButton(string key){
				if(key == "LeftMouseButton") {
					return spades::ui::MouseButton::LeftMouseButton;
				}else if(key == "RightMouseButton") {
					return spades::ui::MouseButton::RightMouseButton;
				}else if(key == "MiddleMouseButton") {
					return spades::ui::MouseButton::MiddleMouseButton;
				}else {
					return spades::ui::MouseButton::None;
				}
			}
			
			private UIElement@ GetMouseActiveElement() {
				if(mouseCapturedElement !is null){
					return mouseCapturedElement;
				}
				UIElement@ elm = RootElement.MouseHitTest(MouseCursorPosition);
				return elm;
			}
			
			private Cursor@ GetCurrentCursor() {
				UIElement@ e = GetMouseActiveElement();
				if(e is null) {
					return DefaultCursor;
				}
				return e.Cursor;
			}
			
			void MouseEvent(float x, float y) {
				MouseCursorPosition = Vector2(
					Clamp(MouseCursorPosition.x + x, 0.f, renderer.ScreenWidth),
					Clamp(MouseCursorPosition.y + y, 0.f, renderer.ScreenHeight)
				);
				
				UIElement@ e = GetMouseActiveElement();
				if(e !is null) {
					e.MouseMove(e.ScreenToClient(MouseCursorPosition));
				}
				
				// check for mouse enter/leave
				if(e is null) {
					@e = RootElement.MouseHitTest(MouseCursorPosition);
				}
				if(e !is mouseHoverElement) {
					if(mouseHoverElement !is null) {
						mouseHoverElement.MouseLeave();
					}
					@mouseHoverElement = e;
					if(e !is null) {
						e.MouseEnter();
					}
				}
			}
			
			void KeyEvent(string key, bool down) {
				if(key == "WheelUp") {
					UIElement@ e = GetMouseActiveElement();
					if(e !is null) {
						e.MouseWheel(-1.f);
					}
					return;
				}
				if(key == "WheelDown") {
					UIElement@ e = GetMouseActiveElement();
					if(e !is null) {
						e.MouseWheel(1.f);
					}
					return;
				}
				
				MouseButton mb = TranslateMouseButton(key);
				if(mb != spades::ui::MouseButton::None) {
					UIElement@ e = GetMouseActiveElement();
					if(e !is null) {
						if(down) {
							@mouseCapturedElement = e;
							if(e.AcceptsFocus) {
								// give keyboard focus, too
								@ActiveElement = e;
							}
							e.MouseDown(mb, e.ScreenToClient(MouseCursorPosition));
						} else {
							// FIXME: release the mouse capture when all button are released?
							@mouseCapturedElement = null;
							e.MouseUp(mb, e.ScreenToClient(MouseCursorPosition));
							MouseEvent(0.f, 0.f);
						}
					}
					return;
				}
				
				{
					UIElement@ e = ActiveElement;
					if(e !is null) {
						if(down) {
							e.KeyDown(key);
						} else {
							e.KeyUp(key);
						}
					}
				}
			}
			
			void CharEvent(string chr) {
				UIElement@ e = ActiveElement;
				if(e !is null) {
					e.KeyPress(chr);
				}
			}
			
			void ProcessHotKey(string key) {
				if(RootElement.Visible) {
					RootElement.HotKey(key);
				}
			}
			
			void PlaySound(string filename) {
				if(audioDevice !is null) {
					
				}
			}
			
			void Render() {
				if(RootElement.Visible) {
					RootElement.Render();
				}
				
				// render cursor
				Cursor@ c = GetCurrentCursor();
				if(c !is null) {
					c.Render(MouseCursorPosition);
				}
			}
		}
		
		enum MouseButton {
			None,
			LeftMouseButton,
			RightMouseButton,
			MiddleMouseButton
		}
		
		class UIElement {
			private UIManager@ manager;
			private UIElement@ parent;
			private UIElement@[] children = {};
			private Font@ fontOverride;
			private Cursor@ cursorOverride;
			
			bool Visible = true;
			
			/** When AcceptsFocus is set to true, this element can be activated when
			 *  it receives a mouse event. */
			bool AcceptsFocus = false;
			
			/** When IsMouseInteractive is set to true, this element receives mouse events. */
			bool IsMouseInteractive = false;
			
			Vector2 Position;
			Vector2 Size;
			
			
			UIElement(UIManager@ manager) {
				@this.manager = manager;
			}
			
			UIElement@ Parent {
				get final { return parent; }
				set final {
					if(value is this.Parent){
						return;
					}
					
					// remove from old container
					if(parent !is null){
						parent.RemoveChild(this);
					}
					
					// add to new container
					if(value !is null){
						value.AddChild(this);
					}
				}
			}
			
			UIManager@ Manager {
				get final { return manager; }
			}
			
			UIElement@[] GetChildren() final {
				return array<spades::ui::UIElement@>(children);
			}
			
			Font@ Font {
				get final {
					if(fontOverride !is null) {
						return fontOverride;
					}
					if(parent is null) {
						return null;
					}
					return parent.Font;
				}
				set {
					@fontOverride = value;
				}
			}
			
			Cursor@ Cursor {
				get final {
					if(cursorOverride !is null) {
						return cursorOverride;
					}
					if(parent is null) {
						return Manager.DefaultCursor;
					}
					return parent.Cursor;
				}
				set {
					@cursorOverride = value;
				}
			}
			
			bool IsVisible {
				get final {
					if(not Visible) return false;
					if(parent is null) return true;
					return parent.IsVisible;
				}
			}
			
			bool IsFocused {
				get final {
					return manager.ActiveElement is this;
				}
			}
			
			void AddChild(UIElement@ element) {
				UIElement@ oldParent = element.Parent;
				if(oldParent is this){
					return;
				}else if(oldParent !is null){
					@element.Parent = null;
				}
				
				children.insertLast(element);
				@element.parent = this;
			}
			
			void RemoveChild(UIElement@ element) {
				int index = children.find(element);
				
				if(index >= 0){
					children.removeAt(index);
					@element.parent = null;
				}
			}
			
			Vector2 ScreenPosition {
				get final {
					if(parent is null) {
						return Position;
					}
					return Position + parent.ScreenPosition;
				}
			}
			
			AABB2 Bounds {
				get final {
					return AABB2(Position, Position + Size);
				}
				set {
					Position = value.min;
					Size = value.max - value.min;
				}
			}
			
			AABB2 ScreenBounds {
				get final {
					Vector2 screenPos = ScreenPosition;
					return AABB2(screenPos, screenPos + Size);
				}
			}
			
			// relativePos is parent relative
			UIElement@ MouseHitTest(Vector2 relativePos) final {
				if(IsMouseInteractive) {
					if(Bounds.Contains(relativePos)) {
						return this;
					}
				}
				
				relativePos -= Position;
				
				UIElement@[] c = children;
				for(int i = c.length() - 1; i >= 0; i--) {
					UIElement@ elem = c[i].MouseHitTest(relativePos);
					if(elem !is null){
						return elem;
					}
				}
				
				return null;
			}
			
			Vector2 ScreenToClient(Vector2 scr) final {
				return scr - ScreenPosition;
			}
			
			void MouseWheel(float delta) {
			}
			void MouseDown(MouseButton button, Vector2 clientPosition) {
			}
			void MouseMove(Vector2 clientPosition) {
			}
			void MouseUp(MouseButton button, Vector2 clientPosition) {
			}
			void MouseEnter() {
			}
			void MouseLeave() {
			}
			
			void KeyDown(string key) {
				manager.ProcessHotKey(key);
			}
			void KeyUp(string key) {
			}
			
			void KeyPress(string text) {
			}
			
			void HotKey(string key) {
				UIElement@[] c = children;
				for(int i = 0, num = c.length(); i < num; i++) {
					if(c[i].Visible) {
						c[i].HotKey(key);
					}
				}
			}
			
			void Render() {
				UIElement@[] c = children;
				for(int i = 0, num = c.length(); i < num; i++) {
					if(c[i].Visible) {
						c[i].Render();
					}
				}
			}
		}
		
		class Cursor {
			private UIManager@ manager;
			private Image@ image;
			private Vector2 hotSpot;
			
			Cursor(UIManager@ manager, Image@ image, Vector2 hotSpot) {
				@this.manager = manager;
				@this.image = image;
				this.hotSpot = hotSpot;
			}
			
			void Render(Vector2 pos) {
				Renderer@ renderer = manager.Renderer;
				renderer.Color = Vector4(1.f, 1.f, 1.f, 1.f);
				renderer.DrawImage(image, Vector2(pos.x - hotSpot.x, pos.y - hotSpot.y));
			}
		}
		
	}
}