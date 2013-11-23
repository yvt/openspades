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
			float Time = 0.f;
			bool IsControlPressed = false;
			bool IsShiftPressed = false;
			
			private UIElement@ mouseCapturedElement;
			private UIElement@ mouseHoverElement;
			
			private Timer@[] timers = {};
			
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
				
				@DefaultCursor = Cursor(this, renderer.RegisterImage("Gfx/Cursor.png"), Vector2(8.f, 8.f));
				MouseCursorPosition = Vector2(renderer.ScreenWidth * 0.5f, renderer.ScreenHeight * 0.5f);
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
				if(key == "Shift") {
					IsShiftPressed = down;
				}
				if(key == "Control") {
					IsControlPressed = down;
				}
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
			
			void AddTimer(Timer@ timer) {
				timers.insertLast(timer);
			}
			
			void RemoveTimer(Timer@ timer) {
				int idx = -1;
				Timer@[]@ t = timers;
				for(int i = t.length - 1; i >= 0; i--){
					if(t[i] is timer) {
						idx = i;
						break;
					}
				}
				if(idx >= 0){
					timers.removeAt(idx);
				}
			}
			
			void PlaySound(string filename) {
				if(audioDevice !is null) {
					// TODO: play sound
				}
			}
			
			void RunFrame(float dt) {
				Timer@[]@ timers = this.timers;
				for(int i = timers.length - 1; i >= 0; i--) {
					timers[i].RunFrame(dt);
				}
				Time += dt;
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
		
		funcdef void TimerTickEventHandler(Timer@);
		class Timer {
			private UIManager@ manager;
			TimerTickEventHandler@ Tick;
			
			/** Minimum interval with which the timer fires. */
			float Interval = 1.f;
			
			bool AutoReset = true;
			
			private float nextDelay;
			
			Timer(UIManager@ manager) {
				@this.manager = manager;
			}
			
			UIManager@ Manager {
				get final {
					return manager;
				}
			}
			
			void OnTick() {
				if(Tick !is null) {
					Tick(this);
				}
			}
			
			/** Called by UIManager. */
			void RunFrame(float dt) {
				nextDelay -= dt;
				if(nextDelay < 0.f) {
					OnTick();
					if(AutoReset) {
						nextDelay = Max(nextDelay + Interval, 0.f);
					} else {
						Stop();
					}
				}
			}
			
			void Start() {
				nextDelay = Interval;
				manager.AddTimer(this);
			}
			
			void Stop() {
				manager.RemoveTimer(this);
			}
			
		}
		
		enum MouseButton {
			None,
			LeftMouseButton,
			RightMouseButton,
			MiddleMouseButton
		}
		
		class UIElementIterator {
			private bool initial = true;
			private UIElement@ e;
			UIElementIterator(UIElement@ parent) {
				@e = parent;
			}
			UIElement@ Current {
				get { return initial ? null : e; }
			}
			bool MoveNext() {
				if(initial) {
					@e = e.FirstChild;
					initial = false;
				} else {
					@e = e.NextSibling;
				}
				return @e !is null;
			}
		}
		
		class UIElement {
			private UIManager@ manager;
			private UIElement@ parent;
			//private UIElement@[] children = {};
			private UIElement@ firstChild, lastChild;
			private UIElement@ prevSibling, nextSibling;
			private Font@ fontOverride;
			private Cursor@ cursorOverride;
			
			bool Visible = true;
			
			/** When AcceptsFocus is set to true, this element can be activated when
			 *  it receives a mouse event. */
			bool AcceptsFocus = false;
			
			/** When IsMouseInteractive is set to true, this element receives mouse events. */
			bool IsMouseInteractive = false;
			
			/** When this is set to true, all mouse interraction outside the client area is
			 * ignored for all sub-elements, reducing the CPU load. The visual is not clipped. */
			bool ClipMouse = true;
			
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
			
			// used by UIElementIterator. Do not use.
			UIElement@ FirstChild {
				get final { return firstChild; }
			}
			
			// used by UIElementIterator. Do not use.
			UIElement@ NextSibling {
				get final { return nextSibling; }
			}
			
			UIElement@[]@ GetChildren() final {
				UIElement@[] elems = {};
				UIElement@ e = firstChild;
				while(e !is null) {
					elems.insertLast(e);
					@e = e.nextSibling;
				}
				return elems;
				//return array<spades::ui::UIElement@>(children);
			}
			
			void AddChild(UIElement@ element) {
				
				UIElement@ oldParent = element.Parent;
				if(oldParent is this){
					return;
				}else if(oldParent !is null){
					@element.Parent = null;
				}
				
				if(firstChild is null) {
					@firstChild = element;
					@lastChild = element;
					@element.parent = this;
				} else {
					@element.prevSibling = @lastChild;
					@lastChild.nextSibling = @element;
					@lastChild = @element;
					@element.parent = this;
				}
				
			/*
				children.insertLast(element);
				@element.parent = this;*/
			}
			
			void RemoveChild(UIElement@ element) {
				if(element.parent !is this) {
					return;
				}
				
				if(firstChild is element) {
					if(lastChild is element) {
						@firstChild = null;
						@lastChild = null;
					} else {
						@firstChild = element.nextSibling;
						@firstChild.prevSibling = null;
					}
				} else if(lastChild is element) {
					@lastChild = element.prevSibling;
					@lastChild.nextSibling = null;
				} else {
					@element.prevSibling.nextSibling = @element.nextSibling;
					@element.nextSibling.prevSibling = @element.prevSibling;
				}
				@element.prevSibling = null;
				@element.nextSibling = null;
				@element.parent = null;
				
			/*
				int index = -1;
				UIElement@[]@ c = children;
				for(int i = c.length - 1; i >= 0; i--) {
					if(c[i] is element) {
						index = i;
						break;
					}
				}
				
				if(index >= 0){
					children.removeAt(index);
					@element.parent = null;
				}*/
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
					OnResized();
				}
			}
			
			AABB2 ScreenBounds {
				get final {
					Vector2 screenPos = ScreenPosition;
					return AABB2(screenPos, screenPos + Size);
				}
			}
			
			void OnResized() {
			}
			
			// relativePos is parent relative
			UIElement@ MouseHitTest(Vector2 relativePos) final {
				if(ClipMouse) {
					if(not Bounds.Contains(relativePos)) {
						return null;
					}
				}
				
				
				Vector2 p = relativePos - Position;
				
				UIElementIterator iterator(this);
				while(iterator.MoveNext()) {
					UIElement@ elem = iterator.Current.MouseHitTest(p);
					if(elem !is null){
						return elem;
					}
				}
				
				if(IsMouseInteractive) {
					if(Bounds.Contains(relativePos)) {
						return this;
					}
				}
				
				return null;
			}
			
			Vector2 ScreenToClient(Vector2 scr) final {
				return scr - ScreenPosition;
			}
			
			void MouseWheel(float delta) {
				if(Parent !is null) {
					Parent.MouseWheel(delta);
				}
			}
			void MouseDown(MouseButton button, Vector2 clientPosition) {
				if(Parent !is null) {
					Parent.MouseDown(button, clientPosition);
				}
			}
			void MouseMove(Vector2 clientPosition) {
				if(Parent !is null) {
					Parent.MouseMove(clientPosition);
				}
			}
			void MouseUp(MouseButton button, Vector2 clientPosition) {
				if(Parent !is null) {
					Parent.MouseUp(button, clientPosition);
				}
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
				UIElementIterator iterator(this);
				while(iterator.MoveNext()) {
					if(iterator.Current.Visible) {
						iterator.Current.HotKey(key);
					}
				}
			}
			
			void Render() {
				UIElementIterator iterator(this);
				while(iterator.MoveNext()) {
					if(iterator.Current.Visible) {
						iterator.Current.Render();
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
		
		
		class UIElementDeque {
			private UIElement@[]@ arr;
			private int startIndex = 0;
			private int count = 0;
			
			UIElementDeque() {
				@arr = array<spades::ui::UIElement@>(4);
			}
			
			private int MapIndex(int idx) const {
				idx += startIndex;
				if(idx >= int(arr.length))
					idx -= int(arr.length);
				return idx;
			}
			
			UIElement@ get_opIndex(int idx) const {
				return arr[MapIndex(idx)];
			}
			void set_opIndex(int idx, UIElement@ e) {
				@arr[MapIndex(idx)] = e;
			}
			
			void Reserve(int c) {
				if(int(arr.length) >= c){
					return;
				}
				int newCount = int(arr.length);
				while(newCount < c) {
					newCount *= 2;
				}
				
				UIElement@[] newarr = array<spades::ui::UIElement@>(newCount);
				UIElement@[] oldarr = arr;
				int len = int(oldarr.length);
				int idx = startIndex;
				for(int i = 0; i < count; i++) {
					@newarr[i] = oldarr[idx];
					idx += 1;
					if(idx >= len) {
						idx = 0;
					}
				}
				@arr = newarr;
				startIndex = 0;
			}
			
			void PushFront(UIElement@ elem) {
				Reserve(count + 1);
				startIndex = (startIndex == 0) ? (arr.length - 1) : (startIndex - 1);
				count++;
				@this.Front = elem;
			}
			
			void PushBack(UIElement@ elem) {
				Reserve(count + 1);
				count++;
				@this.Back = elem;
			}
			
			void PopFront() {
				@this.Front = null;
				startIndex = MapIndex(1);
				count--;
			}
			
			void PopBack() {
				@this.Back = null;
				count--;
			}
			
			int Count {
				get { return count; }
			}
			
			void Clear() {
				@arr = array<spades::ui::UIElement@>(4);
				count = 0;
				startIndex = 0;
			}
			
			UIElement@ Front {
				get const { return arr[startIndex]; }
				set { @arr[startIndex] = value; }
			}
			UIElement@ Back {
				get const { return arr[MapIndex(count - 1)]; }
				set { @arr[MapIndex(count - 1)] = value; }
			}
		}
	}
}