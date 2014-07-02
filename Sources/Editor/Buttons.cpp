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

#include "Buttons.h"

namespace spades { namespace editor {
	
	ButtonBase::ButtonBase(UIManager *m):
	UIElement(m) {
	}
	
	ButtonBase::~ButtonBase() {
	}
	
	void ButtonBase::Activate() {
		if (onActivated)
			onActivated();
	}
	
	void ButtonBase::OnMouseEnter() {
		hover = true;
	}
	
	void ButtonBase::OnMouseLeave() {
		hover = false;
	}
	
	void ButtonBase::OnMouseDown(MouseButton b, const Vector2& p) {
		if (b == MouseButton::Left) {
			pressed = true;
			if (IsAutoRepeat()) {
				Activate();
				nextRepeat = GetManager().GetTime() + .3;
			}
		}
	}
	
	void ButtonBase::OnMouseMove(const Vector2& p) {
		if (pressed) {
			bool newHover = (GetLocalBounds() && p) && HitTestLocal(p);
			if (newHover && !hover && IsAutoRepeat()) {
				Activate();
				nextRepeat = GetManager().GetTime() + .3;
			}
			hover = newHover;
		}
	}
	
	void ButtonBase::OnMouseUp(MouseButton b, const Vector2& p) {
		if (b == MouseButton::Left && pressed) {
			pressed = false;
			if (hover && !IsAutoRepeat()) {
				Activate();
			}
		}
	}
	
	void ButtonBase::RenderClient() {
		if (hover && pressed && IsAutoRepeat() &&
			GetManager().GetTime() > nextRepeat) {
			nextRepeat = GetManager().GetTime() + .1;
			Activate();
		}
	}
	
	ButtonState ButtonBase::GetState() const {
		if (pressed && hover) {
			return ButtonState::Pressed;
		} else if (hover) {
			return ButtonState::Hover;
		} else {
			return ButtonState::Default;
		}
	}
	
	Button::Button(UIManager *m):
	ButtonBase(m) {
	}
	
	Button::~Button() {
		
	}
	
	void Button::RenderClient() {
		auto rt = GetScreenBounds();
		auto r = ToHandle(GetManager().GetRenderer());
		Vector4 color;
		switch (GetState()) {
			case ButtonState::Pressed:
				color = Vector4(.4f, .75f, .95f, 1.f) * Vector4(.8f, .9f, .95f, 1.f);
				break;
			case ButtonState::Hover:
				color = Vector4(.4f, .75f, .95f, 1.f);
				break;
			default:
				color = Vector4(.8f, .8f, .8f, 1.f);
				break;
		}
		
		r->SetColorAlphaPremultiplied(color * Vector4(.7f, .7f, .7f, 1.f));
		r->DrawImage(nullptr, rt);
		
		r->SetColorAlphaPremultiplied(color);
		r->DrawImage(nullptr, rt.Inflate(-1.f));
		
		auto f = ToHandle(GetManager().GetFont());
		auto sz = f->Measure(text);
		
		f->Draw(text, (rt.min + (rt.GetSize() - sz) * .5f).Floor(),
				1.f, Vector4(0, 0, 0, 1));
		
	}
	
} }

