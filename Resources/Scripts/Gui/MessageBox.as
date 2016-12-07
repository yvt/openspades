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
	class MessageBoxScreen: spades::ui::UIElement {

		private float contentsTop, contentsHeight;

		spades::ui::EventHandler@ Closed;
		int ResultIndex = -1;

		private spades::ui::UIElement@ owner;

		MessageBoxScreen(spades::ui::UIElement@ owner, string text, string[]@ buttons, float height = 200.f) {
			super(owner.Manager);
			@this.owner = owner;
			@Font = Manager.RootElement.Font;
			this.Bounds = owner.Bounds;

			float contentsWidth = 700.f;
			float contentsLeft = (Manager.Renderer.ScreenWidth - contentsWidth) * 0.5f;
			contentsHeight = height;
			contentsTop = (Manager.Renderer.ScreenHeight - contentsHeight) * 0.5f;
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.4f);
				label.Bounds = Bounds;
				AddChild(label);
			}
			{
				spades::ui::Label label(Manager);
				label.BackgroundColor = Vector4(0, 0, 0, 0.8f);
				label.Bounds = AABB2(0.f, contentsTop - 13.f, Size.x, contentsHeight + 27.f);
				AddChild(label);
			}
			for (uint i = 0; i < buttons.length; ++i) {
				spades::ui::Button button(Manager);
				button.Caption = buttons[i];
				button.Bounds = AABB2(
					contentsLeft + contentsWidth - (150.f + 10.f) * (buttons.length - i) + 10.f,
					contentsTop + contentsHeight - 30.f
					, 150.f, 30.f);

				MessageBoxScreenButtonHandler handler;
				@handler.screen = this;
				handler.resultIndex = int(i);
				@button.Activated = spades::ui::EventHandler(handler.OnPressed);
				AddChild(button);
			}
			{
				spades::ui::TextViewer viewer(Manager);
				AddChild(viewer);
				viewer.Bounds = AABB2(contentsLeft, contentsTop, contentsWidth, contentsHeight - 40.f);
				viewer.Text = text;
			}
		}

		private void OnClosed() {
			if(Closed !is null) {
				Closed(this);
			}
		}

		void EndDialog(int result) {
			ResultIndex = result;
			Close();
		}

		void Close() {
			owner.Enable = true;
			owner.Parent.RemoveChild(this);
			OnClosed();
		}

		void Run() {
			owner.Enable = false;
			owner.Parent.AddChild(this);
		}

		void Render() {
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;
			Renderer@ r = Manager.Renderer;
			Image@ img = r.RegisterImage("Gfx/White.tga");

			r.ColorNP = Vector4(1, 1, 1, 0.08f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop - 15.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop + contentsHeight + 15.f, size.x, 1.f));
			r.ColorNP = Vector4(1, 1, 1, 0.2f);
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop - 14.f, size.x, 1.f));
			r.DrawImage(img,
				AABB2(pos.x, pos.y + contentsTop + contentsHeight + 14.f, size.x, 1.f));

			UIElement::Render();
		}

	}

	class MessageBoxScreenButtonHandler {
		MessageBoxScreen@ screen;
		int resultIndex;

		void OnPressed(spades::ui::UIElement@) {
			screen.EndDialog(resultIndex);
		}
	}

	class AlertScreen: MessageBoxScreen {
		AlertScreen(spades::ui::UIElement@ owner, string text, float height = 200.f) {
			super(owner, text, array<string> = {_Tr("MessageBox", "OK")}, height);
		}

		void HotKey(string key) {
			if(IsEnabled and (key == "Enter" or key == "Escape")) {
				EndDialog(0);
			} else {
				UIElement::HotKey(key);
			}
		}
	}

	class ConfirmScreen: MessageBoxScreen {
		ConfirmScreen(spades::ui::UIElement@ owner, string text, float height = 200.f) {
			super(owner, text, array<string> = {_Tr("MessageBox", "OK"), _Tr("MessageBox", "Cancel")}, height);
		}

		bool get_Result() {
			return ResultIndex == 0;
		}

		void HotKey(string key) {
			if(IsEnabled and key == "Enter") {
				EndDialog(0);
			} else if(IsEnabled and key == "Escape") {
				EndDialog(1);
			} else {
				UIElement::HotKey(key);
			}
		}
	}
}
