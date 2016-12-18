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

		class Label: UIElement {
			string Text;
			Vector4 BackgroundColor = Vector4(0, 0, 0, 0);
			Vector4 TextColor = Vector4(1, 1, 1, 1);
			Vector2 Alignment = Vector2(0.f, 0.0f);
			float TextScale = 1.f;

			Label(UIManager@ manager) {
				super(manager);
			}
			void Render() {
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;

				if(BackgroundColor.w > 0.f) {
					Image@ img = renderer.RegisterImage("Gfx/White.tga");
					renderer.ColorNP = BackgroundColor;
					renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
				}

				if(Text.length > 0) {
					Font@ font = this.Font;
					string text = this.Text;
					Vector2 txtSize = font.Measure(text) * TextScale;
					Vector2 txtPos;
					txtPos = pos + (size - txtSize) * Alignment;

					font.Draw(text, txtPos, TextScale, TextColor);
				}
			}
		}

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

		class FieldCommand {
			int index;
			string oldString;
			string newString;
		}

		class FieldBase: UIElement {
			bool Dragging = false;
			EventHandler@ Changed;
			string Placeholder;
			int MarkPosition = 0;
			int CursorPosition = 0;
			int MaxLength = 255;
			bool DenyNonAscii = false;

			private string text;
			private FieldCommand@[] history;
			private int historyPos = 0; // index to insert next command

			Vector4 TextColor = Vector4(1.f, 1.f, 1.f, 1.f);
			Vector4 DisabledTextColor = Vector4(1.f, 1.f, 1.f, 0.3f);
			Vector4 PlaceholderColor = Vector4(1.f, 1.f, 1.f, 0.5f);
			Vector4 HighlightColor = Vector4(1.f, 1.f, 1.f, 0.3f);

			Vector2 TextOrigin = Vector2(0.f, 0.f);
			float TextScale = 1.f;

			FieldBase(UIManager@ manager) {
				super(manager);
				IsMouseInteractive = true;
				AcceptsFocus = true;
				@this.Cursor = Cursor(Manager, manager.Renderer.RegisterImage("Gfx/UI/IBeam.png"), Vector2(16.f, 16.f));
			}

			string Text {
				get { return text; }
				set {
					text = value;
					EraseUndoHistory();
				 }
			}

			private void EraseUndoHistory() {
				history.length = 0;
				historyPos = 0;
			}

			private bool CheckCharType(string s) {
				if(DenyNonAscii) {
					for(uint i = 0, len = s.length; i < len; i++) {
						int c = s[i];
						if((c & 0x80) != 0) {
							return false;
						}
					}
				}
				return true;
			}

			void OnChanged() {
				if(Changed !is null) {
					Changed(this);
				}
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
					if(!CheckCharType(value)) {
						return;
					}
					FieldCommand cmd;
					cmd.oldString = this.SelectedText;
					if(cmd.oldString == value) return; // no change
					cmd.newString = value;
					cmd.index = this.SelectionStart;
					RunFieldCommand(cmd, true, true);
				}
			}

			private void RunFieldCommand(FieldCommand@ cmd, bool autoSelect, bool addHistory) {
				text = text.substr(0, cmd.index) + cmd.newString +
					text.substr(cmd.index + cmd.oldString.length);
				if(autoSelect)
					Select(cmd.index, cmd.newString.length);

				if(addHistory) {
					history.length = historyPos;
					history.insertLast(cmd);
					historyPos += 1;
					// limit history length
				}

			}

			private void UndoFieldCommand(FieldCommand@ cmd, bool autoSelect) {
				text = text.substr(0, cmd.index) + cmd.oldString +
					text.substr(cmd.index + cmd.newString.length);
				if(autoSelect)
					Select(cmd.index, cmd.oldString.length);

			}

			private void SetHistoryPos(int index) {
				int p = historyPos;
				FieldCommand@[]@ h = history;
				while(p < index) {
					RunFieldCommand(h[p], true, false);
					p++;
				}
				while(p > index) {
					p--;
					UndoFieldCommand(h[p], true);
				}
				historyPos = p;
			}

			bool Undo() {
				if(historyPos == 0) return false;
				SetHistoryPos(historyPos - 1);
				return true;
			}

			bool Redo() {
				if(historyPos >= int(history.length)) return false;
				SetHistoryPos(historyPos + 1);
				return true;
			}

			AABB2 TextInputRect {
				get {
					Vector2 textPos = TextOrigin;
					Vector2 siz = this.Size;
					string text = Text;
					int cursorPos = CursorPosition;
					Font@ font = this.Font;
					float width = font.Measure(text.substr(0, cursorPos)).x;
					float fontHeight = font.Measure("A").y;
					return AABB2(textPos.x + width, textPos.y,
								 siz.x - textPos.x - width, fontHeight);
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
				int idx = 0;
				for(int i = 1; i <= len; i++) {
					int lastIdx = idx;
					idx = GetByteIndexForString(text, 1, idx);
					float width = font.Measure(text.substr(0, idx)).x;
					if(width > x) {
						if(x < (lastWidth + width) * 0.5f) {
							return lastIdx;
						} else {
							return idx;
						}
					}
					lastWidth = width;
					if(idx >= len) {
						return len;
					}
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
					int pos = CursorPosition;
					int cIdx = GetCharIndexForString(Text, CursorPosition);
					int bIdx = GetByteIndexForString(Text, cIdx - 1);
					Select(bIdx, pos - bIdx);
					SelectedText = "";
				}
				OnChanged();
			}

			void Delete() {
				if(SelectionLength > 0) {
					SelectedText = "";
				} else if(CursorPosition < int(Text.length)) {
					int pos = CursorPosition;
					int cIdx = GetCharIndexForString(Text, CursorPosition);
					int bIdx = GetByteIndexForString(Text, cIdx + 1);
					Select(bIdx, pos - bIdx);
					SelectedText = "";
				}
				OnChanged();
			}

			void Insert(string text) {
				if(!CheckCharType(text)) {
					return;
				}
				string oldText = SelectedText;
				SelectedText = text;

				// if text overflows, deny the insertion
				if((not FitsInBox(Text)) or (int(Text.length) > MaxLength)) {
					SelectedText = oldText;
					return;
				}

				Select(SelectionEnd);
				OnChanged();
			}

			void KeyDown(string key) {
				if(key == "BackSpace") {
					BackSpace();
				}else if(key == "Delete") {
					Delete();
				}else if(key == "Left") {
					if(Manager.IsShiftPressed) {
						int cIdx = GetCharIndexForString(Text, CursorPosition);
						CursorPosition = ClampCursorPosition(GetByteIndexForString(Text, cIdx - 1));
					}else {
						if(SelectionLength == 0) {
							int cIdx = GetCharIndexForString(Text, CursorPosition);
							Select(GetByteIndexForString(Text, cIdx - 1));
						} else {
							Select(SelectionStart);
						}
					}
					return;
				}else if(key == "Right") {
					if(Manager.IsShiftPressed) {
						int cIdx = GetCharIndexForString(Text, CursorPosition);
						CursorPosition = ClampCursorPosition(GetByteIndexForString(Text, cIdx + 1));
					}else {
						if(SelectionLength == 0) {
							int cIdx = GetCharIndexForString(Text, CursorPosition);
							Select(GetByteIndexForString(Text, cIdx + 1));
						} else {
							Select(SelectionEnd);
						}
					}
					return;
				}
				if(Manager.IsControlPressed or
				   Manager.IsMetaPressed /* for OSX; Cmd + [a-z] */) {
					if(key == "A") {
						SelectAll();
						return;
					}else if(key == "V") {
						Manager.Paste(PasteClipboardEventHandler(this.Insert));
					}else if(key == "C") {
						Manager.Copy(this.SelectedText);
					}else if(key == "X") {
						Manager.Copy(this.SelectedText);
						this.SelectedText = "";
						OnChanged();
					}else if(key == "Z") {
						if(Manager.IsShiftPressed){
							if(Redo()) OnChanged();
						}else{
							if(Undo()) OnChanged();
						}
					}else if(key == "W") {
						if(Redo()) {
							OnChanged();
						}
					}
				}
				Manager.ProcessHotKey(key);
			}
			void KeyUp(string key) {
			}

			void KeyPress(string text) {
				if(!(Manager.IsControlPressed or
				     Manager.IsMetaPressed)) {
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
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);

				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.DrawImage(img, AABB2(x, y, w, h));
			}

			void DrawBeam(float x, float y, float h) {
				Renderer@ renderer = Manager.Renderer;
				float pulse = sin(Manager.Time * 5.f);
				pulse = abs(pulse);
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, pulse);

				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.DrawImage(img, AABB2(x - 1.f, y, 2, h));
			}

			void DrawEditingLine(float x, float y, float w, float h) {
				Renderer@ renderer = Manager.Renderer;
				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, .3f);

				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.DrawImage(img, AABB2(x, y + h, w, 2.f));
			}

			void Render() {
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				Font@ font = this.Font;
				Vector2 textPos = TextOrigin + pos;
				string text = Text;

				string composition = this.EditingText;
				int editStart = this.TextEditingRangeStart;
				int editLen = this.TextEditingRangeLength;

				int markStart = SelectionStart;
				int markEnd = SelectionEnd;

				if(composition.length > 0){
					this.SelectedText = "";
					markStart = SelectionStart + editStart;
					markEnd = markStart + editLen;
					text = text.substr(0, SelectionStart) + composition + text.substr(SelectionStart);
				}

				if(text.length == 0){
					if(IsEnabled) {
						font.Draw(Placeholder, textPos, TextScale, PlaceholderColor);
					}
				}else{
					font.Draw(text, textPos, TextScale, IsEnabled ? TextColor : DisabledTextColor);
				}

				if(IsFocused){
					float fontHeight = font.Measure("A").y;

					// draw selection
					int start = markStart;
					int end = markEnd;
					if(end == start) {
						float x = font.Measure(text.substr(0, start)).x;
						DrawBeam(x + textPos.x, textPos.y, fontHeight);
					} else {
						float x1 = font.Measure(text.substr(0, start)).x;
						float x2 = font.Measure(text.substr(0, end)).x;
						DrawHighlight(textPos.x + x1, textPos.y, x2 - x1, fontHeight);
					}

					// draw composition underline
					if(composition.length > 0) {
						start = SelectionStart;
						end = start + composition.length;
						float x1 = font.Measure(text.substr(0, start)).x;
						float x2 = font.Measure(text.substr(0, end)).x;
						DrawEditingLine(textPos.x + x1, textPos.y, x2 - x1, fontHeight);
					}
				}

				UIElement::Render();
			}
		}

		class Field: FieldBase {
			private bool hover;
			Field(UIManager@ manager) {
				super(manager);
				TextOrigin = Vector2(2.f, 2.f);
			}
			void MouseEnter() {
				hover = true;
				FieldBase::MouseEnter();
			}
			void MouseLeave() {
				hover = false;
				FieldBase::MouseLeave();
			}
			void Render() {
				// render background
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.ColorNP = Vector4(0.f, 0.f, 0.f, IsFocused ? 0.3f : 0.1f);
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));

				if(IsFocused) {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
				}else if(hover) {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.1f);
				} else {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.06f);
				}
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
				renderer.DrawImage(img, AABB2(pos.x, pos.y + size.y - 1.f, size.x, 1.f));
				renderer.DrawImage(img, AABB2(pos.x, pos.y + 1.f, 1.f, size.y - 2.f));
				renderer.DrawImage(img, AABB2(pos.x + size.x - 1.f, pos.y + 1.f, 1.f, size.y - 2.f));

				FieldBase::Render();
			}
		}

		enum ScrollBarOrientation {
			Horizontal,
			Vertical
		}

		class ScrollBarBase: UIElement {
			double MinValue = 0.0;
			double MaxValue = 100.0;
			double Value = 0.0;
			double SmallChange = 1.0;
			double LargeChange = 20.0;
			EventHandler@ Changed;

			ScrollBarBase(UIManager@ manager) {
				super(manager);
			}

			void ScrollBy(double delta) {
				ScrollTo(Value + delta);
			}

			void ScrollTo(double val) {
				val = Clamp(val, MinValue, MaxValue);
				if(val == Value) {
					return;
				}
				Value = val;
				OnChanged();
			}

			void OnChanged() {
				if(Changed !is null) {
					Changed(this);
				}
			}

			ScrollBarOrientation Orientation {
				get {
					if(Size.x > Size.y) {
						return spades::ui::ScrollBarOrientation::Horizontal;
					} else {
						return spades::ui::ScrollBarOrientation::Vertical;
					}
				}
			}


		}

		class ScrollBarTrackBar: UIElement {
			private ScrollBar@ scrollBar;
			private bool dragging = false;
			private double startValue;
			private float startCursorPos;
			private bool hover = false;

			ScrollBarTrackBar(ScrollBar@ scrollBar) {
				super(scrollBar.Manager);
				@this.scrollBar = scrollBar;
				IsMouseInteractive = true;
			}

			private float GetCursorPos(Vector2 pos) {
				if(scrollBar.Orientation == spades::ui::ScrollBarOrientation::Horizontal) {
					return pos.x + Position.x;
				} else {
					return pos.y + Position.y;
				}
			}

			void MouseDown(MouseButton button, Vector2 clientPosition) {
				if(button != spades::ui::MouseButton::LeftMouseButton) {
					return;
				}
				if(scrollBar.TrackBarMovementRange < 0.0001f) {
					// immobile
					return;
				}
				dragging = true;
				startValue = scrollBar.Value;
				startCursorPos = GetCursorPos(clientPosition);
			}
			void MouseMove(Vector2 clientPosition) {
				if(dragging) {
					double val = startValue;
					float delta = GetCursorPos(clientPosition) - startCursorPos;
					val += delta * (scrollBar.MaxValue - scrollBar.MinValue) /
						double(scrollBar.TrackBarMovementRange);
					scrollBar.ScrollTo(val);
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

				if(scrollBar.Orientation == spades::ui::ScrollBarOrientation::Horizontal) {
					pos.y += 4.f; size.y -= 8.f;
				} else {
					pos.x += 4.f; size.x -= 8.f;
				}

				if(dragging) {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.4f);
				} else if (hover) {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
				} else {
					renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.1f);
				}
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));
			}
		}

		class ScrollBarFill: ButtonBase {
			private ScrollBarBase@ scrollBar;
			private bool up;

			ScrollBarFill(ScrollBarBase@ scrollBar, bool up) {
				super(scrollBar.Manager);
				@this.scrollBar = scrollBar;
				IsMouseInteractive = true;
				Repeat = true;
				this.up = up;
			}

			void PlayMouseEnterSound() {
				// suppress
			}

			void PlayActivateSound() {
				// suppress
			}

			void Render() {
				// nothing to draw
			}
		}

		class ScrollBarButton: ButtonBase {
			private ScrollBar@ scrollBar;
			private bool up;
			private Image@ image;

			ScrollBarButton(ScrollBar@ scrollBar, bool up) {
				super(scrollBar.Manager);
				@this.scrollBar = scrollBar;
				IsMouseInteractive = true;
				Repeat = true;
				this.up = up;
				@image = Manager.Renderer.RegisterImage("Gfx/UI/ScrollArrow.png");
			}

			void PlayMouseEnterSound() {
				// suppress
			}

			void PlayActivateSound() {
				// suppress
			}

			void Render() {
				Renderer@ r = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				pos += size * 0.5f;
				float siz = image.Width * 0.5f;
				AABB2 srcRect(0.f, 0.f, image.Width, image.Height);

				if(Pressed and Hover) {
					r.ColorNP = Vector4(1.f, 1.f, 1.f, 0.6f);
				} else if (Hover) {
					r.ColorNP = Vector4(1.f, 1.f, 1.f, 0.4f);
				} else {
					r.ColorNP = Vector4(1.f, 1.f, 1.f, 0.2f);
				}

				if(scrollBar.Orientation == spades::ui::ScrollBarOrientation::Horizontal) {
					if(up) {
						r.DrawImage(image,
							Vector2(pos.x + siz, pos.y - siz), Vector2(pos.x + siz, pos.y + siz), Vector2(pos.x - siz, pos.y - siz),
							srcRect);
					} else {
						r.DrawImage(image,
							Vector2(pos.x - siz, pos.y + siz), Vector2(pos.x - siz, pos.y - siz), Vector2(pos.x + siz, pos.y + siz),
							srcRect);
					}
				} else {
					if(up) {
						r.DrawImage(image,
							Vector2(pos.x + siz, pos.y + siz), Vector2(pos.x - siz, pos.y + siz), Vector2(pos.x + siz, pos.y - siz),
							srcRect);
					} else {
						r.DrawImage(image,
							Vector2(pos.x - siz, pos.y - siz), Vector2(pos.x + siz, pos.y - siz), Vector2(pos.x - siz, pos.y + siz),
							srcRect);
					}
				}
			}
		}

		class ScrollBar: ScrollBarBase {

			private ScrollBarTrackBar@ trackBar;
			private ScrollBarFill@ fill1;
			private ScrollBarFill@ fill2;
			private ScrollBarButton@ button1;
			private ScrollBarButton@ button2;

			private float ButtonSize = 16.f;

			ScrollBar(UIManager@ manager) {
				super(manager);

				@trackBar = ScrollBarTrackBar(this);
				AddChild(trackBar);

				@fill1 = ScrollBarFill(this, false);
				@fill1.Activated = EventHandler(this.LargeDown);
				AddChild(fill1);
				@fill2 = ScrollBarFill(this, true);
				@fill2.Activated = EventHandler(this.LargeUp);
				AddChild(fill2);

				@button1 = ScrollBarButton(this, false);
				@button1.Activated = EventHandler(this.SmallDown);
				AddChild(button1);
				@button2 = ScrollBarButton(this, true);
				@button2.Activated = EventHandler(this.SmallUp);
				AddChild(button2);
			}

			private void LargeDown(UIElement@ e) {
				ScrollBy(-LargeChange);
			}
			private void LargeUp(UIElement@ e) {
				ScrollBy(LargeChange);
			}
			private void SmallDown(UIElement@ e) {
				ScrollBy(-SmallChange);
			}
			private void SmallUp(UIElement@ e) {
				ScrollBy(SmallChange);
			}

			void OnChanged() {
				Layout();
				ScrollBarBase::OnChanged();
				Layout();
			}

			void Layout() {
				Vector2 size = Size;
				float tPos = TrackBarPosition;
				float tLen = TrackBarLength;
				if(Orientation == spades::ui::ScrollBarOrientation::Horizontal) {
					button1.Bounds = AABB2(0.f, 0.f, ButtonSize, size.y);
					button2.Bounds = AABB2(size.x - ButtonSize, 0.f, ButtonSize, size.y);
					fill1.Bounds = AABB2(ButtonSize, 0.f, tPos - ButtonSize, size.y);
					fill2.Bounds = AABB2(tPos + tLen, 0.f, size.x - ButtonSize - tPos - tLen, size.y);
					trackBar.Bounds = AABB2(tPos, 0.f, tLen, size.y);
				} else {
					button1.Bounds = AABB2(0.f, 0.f, size.x, ButtonSize);
					button2.Bounds = AABB2(0.f, size.y - ButtonSize, size.x, ButtonSize);
					fill1.Bounds = AABB2(0.f, ButtonSize, size.x, tPos - ButtonSize);
					fill2.Bounds = AABB2(0.f, tPos + tLen, size.x, size.y - ButtonSize - tPos - tLen);
					trackBar.Bounds = AABB2(0.f, tPos, size.x, tLen);
				}
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
					return Length - ButtonSize - ButtonSize;
				}
			}

			float TrackBarLength {
				get {
					return Max(TrackBarAreaLength *
						(LargeChange / (MaxValue - MinValue + LargeChange)), 40.f);
				}
			}

			float TrackBarMovementRange {
				get {
					return TrackBarAreaLength - TrackBarLength;
				}
			}

			float TrackBarPosition {
				get {
					if(MaxValue == MinValue) {
						return ButtonSize;
					}
					return float((Value - MinValue) / (MaxValue - MinValue) * TrackBarMovementRange) + ButtonSize;
				}
			}

			void Render() {
				Layout();

				ScrollBarBase::Render();
			}
		}


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

		class ListViewModel {
			int NumRows { get { return 0; } }
			UIElement@ CreateElement(int row) { return null; }
			void RecycleElement(UIElement@ elem) {}
		}

		/** Simple virtual stack panel implementation. */
		class ListViewBase: UIElement {
			private ScrollBar@ scrollBar;
			private ListViewModel@ model;
			float RowHeight = 24.f;
			float ScrollBarWidth = 16.f;
			private UIElementDeque items;
			private int loadedStartIndex = 0;

			ListViewBase(UIManager@ manager) {
				super(manager);
				@scrollBar = ScrollBar(Manager);
				scrollBar.Bounds = AABB2();
				AddChild(scrollBar);
				IsMouseInteractive = true;

				@scrollBar.Changed = EventHandler(this.OnScrolled);
				@model = ListViewModel();
			}

			private void OnScrolled(UIElement@ sender) {
				Layout();
			}

			int NumVisibleRows {
				get final {
					return int(floor(Size.y / RowHeight));
				}
			}

			int MaxTopRowIndex {
				get final {
					return Max(0, model.NumRows - NumVisibleRows);
				}
			}

			int TopRowIndex {
				get final {
					int idx = int(floor(scrollBar.Value + 0.5));
					return Clamp(idx, 0, MaxTopRowIndex);
				}
			}

			void OnResized() {
				Layout();
				UIElement::OnResized();
			}

			void Layout() {
				scrollBar.MaxValue = double(MaxTopRowIndex);
				scrollBar.ScrollTo(scrollBar.Value); // ensures value is in range
				scrollBar.LargeChange = double(NumVisibleRows);

				int numRows = model.NumRows;

				// load items
				int visibleStart = TopRowIndex;
				int visibleEnd = Min(visibleStart + NumVisibleRows, numRows);
				int loadedStart = loadedStartIndex;
				int loadedEnd = loadedStartIndex + items.Count;

				if(items.Count == 0 or visibleStart >= loadedEnd or visibleEnd <= loadedStart) {
					// full reload
					UnloadAll();
					for(int i = visibleStart; i < visibleEnd; i++) {
						items.PushBack(model.CreateElement(i));
						AddChild(items.Back);
					}
					loadedStartIndex = visibleStart;
				} else {
					while(loadedStart < visibleStart) {
						RemoveChild(items.Front);
						model.RecycleElement(items.Front);
						items.PopFront();
						loadedStart++;
					}
					while(loadedEnd > visibleEnd) {
						RemoveChild(items.Back);
						model.RecycleElement(items.Back);
						items.PopBack();
						loadedEnd--;
					}
					while(visibleStart < loadedStart) {
						loadedStart--;
						items.PushFront(model.CreateElement(loadedStart));
						AddChild(items.Front);
					}
					while(visibleEnd > loadedEnd) {
						items.PushBack(model.CreateElement(loadedEnd));
						AddChild(items.Back);
						loadedEnd++;
					}
					loadedStartIndex = loadedStart;
				}

				// relayout items
				UIElementDeque@ items = this.items;
				int count = items.Count;
				float y = 0.f;
				float w = ItemWidth;
				for(int i = 0; i < count; i++){
					items[i].Bounds = AABB2(0.f, y, w, RowHeight);
					y += RowHeight;
				}

				// move scroll bar
				scrollBar.Bounds = AABB2(Size.x - ScrollBarWidth, 0.f, ScrollBarWidth, Size.y);
			}

			float ItemWidth {
				get {
					return Size.x - ScrollBarWidth;
				}
			}

			void MouseWheel(float delta) {
				scrollBar.ScrollBy(delta);
			}

			void Reload() {
				UnloadAll();
				Layout();
			}

			private void UnloadAll() {
				UIElementDeque@ items = this.items;
				int count = items.Count;
				for(int i = 0; i < count; i++){
					RemoveChild(items[i]);
					model.RecycleElement(items[i]);
				}
				items.Clear();
			}

			ListViewModel@ Model {
				get final { return model; }
				set {
					if(model is value) {
						return;
					}
					UnloadAll();
					@model = value;
					Layout();
				}
			}

			void ScrollToTop() {
				scrollBar.ScrollTo(0.0);
			}

			void ScrollToEnd() {
				scrollBar.ScrollTo(scrollBar.MaxValue);
			}
		}

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

		class ListView: ListViewBase {
			ListView(UIManager@ manager) {
				super(manager);
			}
			void Render() {
				// render background
				Renderer@ renderer = Manager.Renderer;
				Vector2 pos = ScreenPosition;
				Vector2 size = Size;
				Image@ img = renderer.RegisterImage("Gfx/White.tga");
				renderer.ColorNP = Vector4(0.f, 0.f, 0.f, 0.2f);
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, size.y));

				renderer.ColorNP = Vector4(1.f, 1.f, 1.f, 0.06f);
				renderer.DrawImage(img, AABB2(pos.x, pos.y, size.x, 1.f));
				renderer.DrawImage(img, AABB2(pos.x, pos.y + size.y - 1.f, size.x, 1.f));
				renderer.DrawImage(img, AABB2(pos.x, pos.y + 1.f, 1.f, size.y - 2.f));
				renderer.DrawImage(img, AABB2(pos.x + size.x - 1.f, pos.y + 1.f, 1.f, size.y - 2.f));

				ListViewBase::Render();
			}
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
