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

#include "Fields.h"
#include <Imports/SDL.h>

namespace spades { namespace editor {
	
	FieldBase::FieldBase(UIManager *m):
	UIElement(m),
	cursorPos(0),
	markPos(0) {
		nextHistoryPos = history.begin();
		
		cursor.image = GetManager().GetRenderer()->RegisterImage("Gfx/UI/IBeam.png");
		cursor.hotSpot = Vector2(16, 16);
	}
	
	FieldBase::~FieldBase() {
		
	}
	
	AABB2 FieldBase::GetTextBounds() {
		return GetLocalBounds();
	}
	
	AABB2 FieldBase::GetTextInputRect() {
		return GetCaretPos(cursorPos, 0);
	}
	
	Vector4 FieldBase::GetTextColor() {
		return Vector4(0, 0, 0, 1);
	}
	
	Cursor FieldBase::GetCursor() {
		return cursor;
	}
	
	Vector2 FieldBase::GetTextPos() {
		auto bounds = GetTextBounds();
		float fh = GetFont()->Measure("A").y;
		return Vector2(bounds.min.x, bounds.min.y + (bounds.GetHeight() - fh) * .5f).Floor();
	}
	
	AABB2 FieldBase::GetCaretPos(int index,
								int length,
								 bool visual) {
		auto txt = text;
		if (visual) {
			txt.insert(cursorPos, editingText);
		}
		index = std::max(index, 0);
		int index2 = std::max(index + length, 0);
		auto s = txt.substr(0, index);
		auto s2 = txt.substr(0, index2);
		auto sz = GetFont()->Measure(s);
		auto sz2 = index == index2 ? sz : GetFont()->Measure(s2);
		auto pos = GetTextPos();
		return AABB2(GetTextBounds().min.x + sz.x, pos.y,
					 sz2.x - sz.x, sz.y);
	}
	
	int FieldBase::GetCharacterIndexAt(const Vector2& pt) {
		std::size_t index = 0;
		auto px = pt.x - GetTextPos().x;
		if (px <= 0.f) return 0;
		
		float x = 0.f;
		
		while (index < text.size()) {
			std::size_t sz = 0;
			GetCodePointFromUTF8String(text, index, &sz);
			auto subs = text.substr(index, sz);
			float w = GetFont()->Measure(subs).x;
			if (px < x + w * .5f) return index;
			x += w;
			index += sz;
		}
		return text.size();
	}
	
	void FieldBase::AddCommandToHistory(const Command& cmd) {
		history.erase(nextHistoryPos, history.end());
		history.emplace_back(cmd);
		nextHistoryPos = history.end();
	}
	
	int FieldBase::GetSelectionStart() {
		return std::min(cursorPos, markPos);
	}
	
	int FieldBase::GetSelectionEnd() {
		return std::max(cursorPos, markPos);
	}
	
	int FieldBase::GetSelectionLength() {
		return GetSelectionEnd() - GetSelectionStart();
	}
	
	void FieldBase::Select(int start, int len) {
		markPos = start;
		cursorPos = len + start;
	}
	
	void FieldBase::MoveCursor(int i) {
		if (i == 0) return;
		if (i > 0) {
			std::size_t p = 0;
			GetCodePointFromUTF8String(text, cursorPos, &p);
			cursorPos += p;
			return MoveCursor(i - 1);
		} else {
			for (int i = cursorPos - 1; i >= 0 && i >= cursorPos - 6; --i) {
				std::size_t p = 0;
				if (i == cursorPos -1 && (text[i] & 0x80)) continue;
				GetCodePointFromUTF8String(text, i, &p);
				if (p == cursorPos - i) {
					cursorPos = i;
					break;
				}
			}
			
			return MoveCursor(i + 1);
		}
	}
	
	std::string FieldBase::GetSelectedText() {
		int startPos = std::min(markPos, cursorPos);
		int endPos = std::max(markPos, cursorPos);
		return text.substr(startPos, endPos - startPos);
	}
	
	void FieldBase::SetSelectedText(const std::string &text,
									bool selectNext) {
		int startPos = std::min(markPos, cursorPos);
		if (text == GetSelectedText()) return;
		
		Command cmd;
		cmd.oldString = GetSelectedText();
		cmd.newString = text;
		cmd.index = startPos;
		ApplyCommand(cmd, true);
		
		if (GetCaretPos(this->text.size(), this->text.size()).max.x >
			GetTextBounds().max.x) {
			// overflow!
			RevertCommand(cmd);
			return;
		}
		
		AddCommandToHistory(cmd);
	}
	
	void FieldBase::SetText(const std::string &s) {
		text = s;
		history.clear();
		markPos = cursorPos = 0;
	}
	
	void FieldBase::RenderClient() {
		auto pos = GetScreenBounds().min;
		auto f = ToHandle(GetFont());
		
		auto s = text;
		s.insert(cursorPos, editingText);
		
		f->Draw(s, GetTextPos() + pos,
				1.f, GetTextColor());
		
		auto r = ToHandle(GetManager().GetRenderer());
		
		if (HasKeyboardFocus()) {
			int startPos = std::min(markPos, cursorPos);
			int endPos = std::max(markPos, cursorPos);
			if (!editingText.empty()) {
				startPos = cursorPos + editingStart;
				endPos = startPos + editingLen;
			}
			auto rt = GetCaretPos(startPos,
								  endPos - startPos,
								  true);
			if (rt.GetWidth() == 0.f) {
				rt.max.x += 1.f;
				
				double d = GetManager().GetTime();
				d -= floor(d);
				
				auto f = static_cast<float>(d);
				f = sinf(f * M_PI);
				r->SetColorAlphaPremultiplied(Vector4(0,0,0,1) * f);
			} else {
				r->SetColorAlphaPremultiplied(Vector4(.2,.5,.9,1) * .5f);
			}
			
			r->DrawImage(nullptr, rt.Translated(pos));
		}
	}
	
	void FieldBase::OnMouseEnter() { }
	
	void FieldBase::OnMouseLeave() { }
	
	void FieldBase::OnMouseDown(MouseButton b,
								const Vector2& p) {
		if (b == MouseButton::Left) {
			pressed = true;
			cursorPos = GetCharacterIndexAt(p);
			if (!GetManager().GetKeyModifierState(KeyModifier::Shift)) {
				markPos = cursorPos;
			}
		}
		UIElement::OnMouseDown(b, p);
	}
	void FieldBase::OnMouseMove(const Vector2& p) {
		if (pressed) {
			cursorPos = GetCharacterIndexAt(p);
		}
		UIElement::OnMouseMove(p);
	}
	void FieldBase::OnMouseUp(MouseButton b, const Vector2& p) {
		if (pressed && b == MouseButton::Left) {
			pressed = false;
		}
		UIElement::OnMouseUp(b, p);
	}
	
	void FieldBase::OnTextInputEvent(const std::string &text) {
		SetSelectedText(text, true);
		editingText.clear();
	}
	
	namespace {
		int CharacterIndexToBytes(const std::string& s,
								  int index) {
			std::size_t p = 0;
			while (p < s.size() && index > 0){
				std::size_t sz = 0;
				GetCodePointFromUTF8String(s, p, &sz);
				p += sz;
				--index;
			}
			return static_cast<int>(p);
		}
	}
	
	void FieldBase::OnTextEditingEvent(const std::string &text,
									   int start, int len) {
		editingText = text;
		editingStart = CharacterIndexToBytes(text, start);
		editingLen = CharacterIndexToBytes(text, start + len) - editingStart;
	}
	
	void FieldBase::OnKeyDown(const std::string &key) {
		if (!editingText.empty()) return;
		if (key == "BackSpace") {
			if (GetSelectionLength() > 0) {
				SetSelectedText("", false);
			} else if(GetSelectionStart() > 0) {
				MoveCursor(-1);
				SetSelectedText("", false);
			}
		} else if (key == "Delete") {
			if (GetSelectionLength() > 0) {
				SetSelectedText("", false);
			} else if(GetSelectionEnd() < text.size()) {
				MoveCursor(+1);
				SetSelectedText("", false);
			}
		} else if (key == "Left") {
			MoveCursor(-1);
			if (!GetManager().GetKeyModifierState(KeyModifier::Shift)) {
				markPos = cursorPos;
			}
		} else if (key == "Right") {
			MoveCursor(1);
			if (!GetManager().GetKeyModifierState(KeyModifier::Shift)) {
				markPos = cursorPos;
			}
		} else if (key == "Up") {
			cursorPos = 0;
			if (!GetManager().GetKeyModifierState(KeyModifier::Shift)) {
				markPos = cursorPos;
			}
		} else if (key == "Down") {
			cursorPos = text.size();
			if (!GetManager().GetKeyModifierState(KeyModifier::Shift)) {
				markPos = cursorPos;
			}
		} else if (key == "C" && (GetManager().GetKeyModifierState(KeyModifier::Control) || GetManager().GetKeyModifierState(KeyModifier::Meta))) {
			auto s = GetSelectedText();
			if (!s.empty()) {
			    SDL_SetClipboardText(s.c_str());
		    }
	    } else if (key == "X" && (GetManager().GetKeyModifierState(KeyModifier::Control) || GetManager().GetKeyModifierState(KeyModifier::Meta))) {
 		    auto s = GetSelectedText();
 		    if (!s.empty()) {
	 		    SDL_SetClipboardText(s.c_str());
				SetSelectedText("", false);
		    }
	    } else if (key == "V" && (GetManager().GetKeyModifierState(KeyModifier::Control) || GetManager().GetKeyModifierState(KeyModifier::Meta))) {
 		    char *c = SDL_GetClipboardText();
			if (c) {
				std::string s = c; SDL_free(c);
				if (!s.empty())
					SetSelectedText(s, true);
			}
	    }
	}
	
	void FieldBase::OnLeave() {
		editingText.clear();
	}
	
	void FieldBase::ApplyCommand(const Command& c,
								 bool selectNext) {
		text.replace(c.index, c.oldString.size(),
					 c.newString);
		if (selectNext) {
			markPos = static_cast<int>(c.index + c.newString.size());
			cursorPos = markPos;
		} else {
			markPos = static_cast<int>(c.index);
			cursorPos = static_cast<int>(markPos + c.newString.size());
		}
	}
	
	void FieldBase::RevertCommand(const Command& c) {
		text.replace(c.index, c.newString.size(),
					 c.oldString);
		markPos = static_cast<int>(c.index);
		cursorPos = static_cast<int>(markPos + c.oldString.size());
	}
	
#pragma mark - Field
	
	Field::Field(UIManager *m):
	FieldBase(m) { }
	
	Field::~Field() { }
	
	AABB2 Field::GetTextBounds() {
		return FieldBase::GetTextBounds().Inflate(-1);
	}
	
	void Field::RenderClient() {
		auto rt = GetScreenBounds();
		auto r = ToHandle(GetManager().GetRenderer());
		
		r->SetColorAlphaPremultiplied(Vector4(.7, .7, .7, 1));
		r->DrawImage(nullptr, rt);
		r->SetColorAlphaPremultiplied(Vector4(1,1,1,1));
		r->DrawImage(nullptr, rt.Inflate(-1));
		
		FieldBase::RenderClient();
	}
	
} }

