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
	class AlertScreen: spades::ui::UIElement {
		
		float contentsTop, contentsHeight;
		
		spades::ui::EventHandler@ Closed;
		spades::ui::UIElement@ owner;
		
		AlertScreen(spades::ui::UIElement@ owner, string text, float height = 200.f) {
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
			{
				spades::ui::Button button(Manager);
				button.Caption = _Tr("MessageBox", "OK");
				button.Bounds = AABB2(
					contentsLeft + contentsWidth - 150.f, 
					contentsTop + contentsHeight - 30.f
					, 150.f, 30.f);
				button.Activated = spades::ui::EventHandler(this.OnOkPressed);
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
		
		void Close() {
			owner.Enable = true;
			owner.Parent.RemoveChild(this);
		}
		
		void Run() {
			owner.Enable = false;
			owner.Parent.AddChild(this);
		}
		
		private void OnOkPressed(spades::ui::UIElement@ sender) {
			Close();
		}
		
		void HotKey(string key) {
			if(IsEnabled and (key == "Enter" or key == "Escape")) {
				Close();
			} else {
				UIElement::HotKey(key);
			}
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
}
