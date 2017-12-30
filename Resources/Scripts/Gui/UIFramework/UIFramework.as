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
		funcdef void PasteClipboardEventHandler(string text);

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
			bool IsAltPressed = false;
			bool IsMetaPressed = false;

			// IME (Input Method Editor) support
			string EditingText;
			int EditingStart = 0;
			int EditingLength = 0;

			private UIElement@ mouseCapturedElement;
			private UIElement@ mouseHoverElement;

			private PasteClipboardEventHandler@ clipboardEventHandler;

			private Timer@[] timers = {};
			private KeyRepeatManager keyRepeater;
			private KeyRepeatManager charRepeater;

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

				@DefaultCursor = Cursor(this, renderer.RegisterImage("Gfx/UI/Cursor.png"), Vector2(8.f, 8.f));
				MouseCursorPosition = Vector2(renderer.ScreenWidth * 0.5f, renderer.ScreenHeight * 0.5f);

				@keyRepeater.handler = KeyRepeatEventHandler(this.HandleKeyInner);
				@charRepeater.handler = KeyRepeatEventHandler(this.HandleCharInner);
			}

			private MouseButton TranslateMouseButton(string key){
				if(key == "LeftMouseButton") {
					return spades::ui::MouseButton::LeftMouseButton;
				}else if(key == "RightMouseButton") {
					return spades::ui::MouseButton::RightMouseButton;
				}else if(key == "MiddleMouseButton") {
					return spades::ui::MouseButton::MiddleMouseButton;
				}else if(key == "MouseButton4") {
					return spades::ui::MouseButton::MouseButton4;
				}else if(key == "MouseButton5") {
					return spades::ui::MouseButton::MouseButton5;
				}else {
					return spades::ui::MouseButton::None;
				}
			}

			private UIElement@ GetMouseActiveElement() {
				if(mouseCapturedElement !is null){
					return mouseCapturedElement;
				}
				if(not RootElement.Enable) {
					return null;
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

			void WheelEvent(float x, float y) {
				UIElement@ e = GetMouseActiveElement();
				if(e !is null) {
					e.MouseWheel(y);
				}
			}

			private void MouseEventDone() {
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

			void MouseEvent(float x, float y) {
				/*
				MouseCursorPosition = Vector2(
					Clamp(MouseCursorPosition.x + x, 0.f, renderer.ScreenWidth),
					Clamp(MouseCursorPosition.y + y, 0.f, renderer.ScreenHeight)
				);
				*/
				// in current version, absolute mouse mode is supported.
				MouseCursorPosition = Vector2(
					Clamp(x, 0.f, renderer.ScreenWidth),
					Clamp(y, 0.f, renderer.ScreenHeight)
				);

				MouseEventDone();
			}

			// forces key repeat to stop.
			void KeyPanic() {
				keyRepeater.KeyUp();
				charRepeater.KeyUp();
				EditingText = "";
				EditingStart = 0;
				EditingLength = 0;
				IsShiftPressed = false;
				IsControlPressed = false;
				IsAltPressed = false;
				IsMetaPressed = false;
			}

			void KeyEvent(string key, bool down) {
				if(key.length == 0){
					if(!down) {
						keyRepeater.KeyUp();
						charRepeater.KeyUp();
					}
					return;
				}
				if(key == "Shift") {
					IsShiftPressed = down;
				}
				if(key == "Control") {
					IsControlPressed = down;
				}
				if(key == "Alt") {
					IsAltPressed = down;
				}
				if(key == "Meta") {
					IsMetaPressed = down;
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
							MouseEventDone();
						}
					}
					return;
				}

				if(down) {
					HandleKeyInner(key);
					keyRepeater.KeyDown(key);
				}else{
					keyRepeater.KeyUp();
					charRepeater.KeyUp();
					UIElement@ e = ActiveElement;
					if(e !is null) {
						e.KeyUp(key);
					}
				}
			}

			void TextInputEvent(string chr) {
				charRepeater.KeyDown(chr);
				HandleCharInner(chr);
			}

			void TextEditingEvent(string chr, int start, int len) {
				EditingText = chr;
				EditingStart = GetByteIndexForString(chr, start);
				EditingLength = GetByteIndexForString(chr, len, EditingStart) - EditingStart;
			}

			bool AcceptsTextInput {
				get {
					if(ActiveElement is null) {
						EditingText = "";
						EditingStart = 0;
						EditingLength = 0;
					}
					return ActiveElement !is null;
				}
			}

			AABB2 TextInputRect {
				get {
					UIElement@ e = ActiveElement;
					if(e !is null) {
						AABB2 rt = e.TextInputRect;
						Vector2 off = e.ScreenPosition;
						rt.min += off;
						rt.max += off;
						return rt;
					}else{
						return AABB2();
					}
				}
			}

			private void HandleKeyInner(string key) {
				{
					UIElement@ e = ActiveElement;
					if(EditingText.length > 0) {
						// now text is being composed by IME.
						// ignore some keys to resolve confliction.
						if(key == "Escape" || key == "BackSpace" || key == "Left" || key == "Right" ||
						   key == "Space" || key == "Enter" || key == "Up" || key == "Down" ||
						   key == "Tab") {
						   	return;
						}
					}
					if(e !is null) {
						e.KeyDown(key);
					} else {
						ProcessHotKey(key);
					}
				}
			}

			private void HandleCharInner(string chr) {
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
					audioDevice.PlayLocal(audioDevice.RegisterSound(filename), AudioParam());
				}
			}

			void RunFrame(float dt) {
				if(ActiveElement !is null) {
					if(not (ActiveElement.IsVisible and ActiveElement.IsEnabled)) {
						@ActiveElement = null;
					}
				}

				if(clipboardEventHandler !is null) {
					if(GotClipboardData()) {
						clipboardEventHandler(GetClipboardData());
						@clipboardEventHandler = null;
					}
				}

				Timer@[]@ timers = this.timers;
				for(int i = timers.length - 1; i >= 0; i--) {
					timers[i].RunFrame(dt);
				}
				keyRepeater.RunFrame(dt);
				charRepeater.RunFrame(dt);
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

			void Copy(string text) {
				SetClipboardData(text);
			}

			void Paste(PasteClipboardEventHandler@ handler) {
				RequestClipboardData();
				@clipboardEventHandler = handler;
			}
		}

		funcdef void KeyRepeatEventHandler(string key);
		class KeyRepeatManager {
			string lastKey;
			float nextDelay;
			KeyRepeatEventHandler@ handler;

			void KeyDown(string key) {
				lastKey = key;
				nextDelay = 0.2f;
			}
			void KeyUp() {
				lastKey = "";
			}

			void RunFrame(float dt) {
				if(lastKey.length == 0)
					return;
				nextDelay -= dt;
				if(nextDelay < 0.f) {
					handler(lastKey);
					nextDelay = Max(nextDelay + 0.06f, 0.f);
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
			MiddleMouseButton,
			MouseButton4,
			MouseButton5
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

		class UIElementReverseIterator {
			private bool initial = true;
			private UIElement@ e;
			UIElementReverseIterator(UIElement@ parent) {
				@e = parent;
			}
			UIElement@ Current {
				get { return initial ? null : e; }
			}
			bool MoveNext() {
				if(initial) {
					@e = e.LastChild;
					initial = false;
				} else {
					@e = e.PrevSibling;
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

			EventHandler@ MouseEntered;
			EventHandler@ MouseLeft;

			bool Visible = true;
			bool Enable = true;

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
			UIElement@ LastChild {
				get final { return lastChild; }
			}

			// used by UIElementIterator. Do not use.
			UIElement@ NextSibling {
				get final { return nextSibling; }
			}
			// used by UIElementIterator. Do not use.
			UIElement@ PrevSibling {
				get final { return prevSibling; }
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

			void AppendChild(UIElement@ element) {

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
			}
			void PrependChild(UIElement@ element) {

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
					@element.nextSibling = @firstChild;
					@firstChild.prevSibling = @element;
					@firstChild = @element;
					@element.parent = this;
				}
			}

			void AddChild(UIElement@ element) {
				AppendChild(element);
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
					if(parent is null) return this is Manager.RootElement;
					return parent.IsVisible;
				}
			}

			bool IsEnabled {
				get final {
					if(not Enable) return false;
					if(parent is null) return true;
					return parent.IsEnabled;
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

			// IME supports

			AABB2 TextInputRect {
				get {
					return AABB2(0.f, 0.f, Size.x, Size.y);
				}
			}

			int TextEditingRangeStart {
				get final {
					if(this.IsFocused) {
						return Manager.EditingStart;
					}else{
						return 0;
					}
				}
			}

			int TextEditingRangeLength {
				get final {
					if(this.IsFocused) {
						return Manager.EditingLength;
					}else{
						return 0;
					}
				}
			}

			string EditingText {
				get final {
					if(this.IsFocused) {
						return Manager.EditingText;
					}else{
						return "";
					}
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

				UIElementReverseIterator iterator(this);
				while(iterator.MoveNext()) {
					UIElement@ e = iterator.Current;
					if(not (e.Visible and e.Enable)) {
						continue;
					}
					UIElement@ elem = e.MouseHitTest(p);
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
				if(MouseEntered !is null) {
					MouseEntered(this);
				}
			}
			void MouseLeave() {
				if(MouseLeft !is null) {
					MouseLeft(this);
				}
			}

			void KeyDown(string key) {
				manager.ProcessHotKey(key);
			}
			void KeyUp(string key) {
			}

			void KeyPress(string text) {
			}

			void HotKey(string key) {
				UIElementReverseIterator iterator(this);
				while(iterator.MoveNext()) {
					if(iterator.Current.Visible and iterator.Current.Enable) {
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
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 1.f);
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