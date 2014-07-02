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

#pragma once

#include "UIElement.h"
#include <functional>

namespace spades { namespace editor {
	class FieldBase: public UIElement {
		Cursor cursor;
		bool pressed = false;
		
		struct Command {
			std::size_t index;
			std::string oldString;
			std::string newString;
		};
		std::string text;
		std::list<Command> history;
		decltype(history)::iterator nextHistoryPos;
		Vector2 GetTextPos();
		
		int markPos, cursorPos;
		
		std::string editingText;
		int editingStart = 0, editingLen = 0;
		
		std::string GetSelectedText();
		void SetSelectedText(const std::string&,
							 bool selectNext);
		
		void MoveCursor(int);
		
		void AddCommandToHistory(const Command&);
		void ApplyCommand(const Command&,
						  bool selectNext = false);
		void RevertCommand(const Command&);
		
	protected:
		~FieldBase();
		
		virtual AABB2 GetTextBounds();
		virtual Vector4 GetTextColor();
		void RenderClient() override;
		void OnMouseEnter() override;
		void OnMouseLeave() override;
		void OnMouseDown(MouseButton, const Vector2&) override;
		void OnMouseUp(MouseButton, const Vector2&) override;
		void OnMouseMove(const Vector2&) override;
		void OnTextInputEvent(const std::string&) override;
		void OnTextEditingEvent(const std::string&,
								int start, int len) override;
		void OnKeyDown(const std::string&) override;
		void OnLeave() override;
	public:
		FieldBase(UIManager *);
		
		void SetText(const std::string&);
		std::string GetText() const { return text; }
		
		int GetSelectionStart();
		int GetSelectionEnd();
		int GetSelectionLength();
		void Select(int start, int len);
		
		bool AcceptsKeyboardFocus() override { return true; }
		AABB2 GetTextInputRect() override;
		Cursor GetCursor() override;
		
		AABB2 GetCaretPos(int start, int length,
						  bool visual = false);
		int GetCharacterIndexAt(const Vector2&);
		
	};
	
	class Field: public FieldBase {
	protected:
		~Field();
		void RenderClient() override;
		AABB2 GetTextBounds() override;
	public:
		Field(UIManager *);
		
		
	};
} }
