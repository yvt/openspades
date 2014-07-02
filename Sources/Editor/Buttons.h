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
	
	enum class ButtonState {
		Default,
		Hover,
		Pressed
	};
	
	class ButtonBase: public UIElement {
		bool hover = false;
		bool pressed = false;
		bool autoRepeat = false;
		double nextRepeat = 0.;
		
		std::function<void()> onActivated;
	protected:
		~ButtonBase();
		void RenderClient() override;
		void OnMouseEnter() override;
		void OnMouseLeave() override;
		void OnMouseDown(MouseButton, const Vector2&) override;
		void OnMouseUp(MouseButton, const Vector2&) override;
		void OnMouseMove(const Vector2&) override;
	public:
		ButtonBase(UIManager *);
		
		void Activate();
		
		ButtonState GetState() const;
		
		void SetAutoRepeat(bool b) { autoRepeat = b; }
		bool IsAutoRepeat() const { return autoRepeat; }
		
		void SetActivateHandler(const std::function<void()>& f) {
			onActivated = f;
		}
	};
	
	class Button: public ButtonBase {
		std::string text;
	protected:
		~Button();
		void RenderClient() override;
	public:
		Button(UIManager *);
		
		std::string GetText() const { return text; }
		void SetText(const std::string& s) { text = s; }
	};
	
	
} }

