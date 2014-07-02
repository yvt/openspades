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

#include "MainView.h"
#include "Editor.h"

namespace spades { namespace editor {
	
	MainView::MainView(UIManager *manager,
					   Editor& editor):
	UIElement(manager),
	editor(editor),
	dragMove(false),
	dragLastPos(0, 0)
	{
		SPAssert(manager);
		SetBounds(manager->GetScreenBounds());
	}
	
	MainView::~MainView() {
		
	}
	
	void MainView::OnMouseDown(MouseButton button,
							   const Vector2& pos) {
		if (button == MouseButton::Right) {
			dragMove = true;
			dragLastPos = pos;
		}
	}
	
	void MainView::OnMouseMove(const Vector2& pos) {
		if (dragMove) {
			OnMouseWheel((pos - dragLastPos) * .5f * Vector2(1, -1));
			dragLastPos = pos;
		}
	}
	
	void MainView::OnMouseUp(MouseButton button,
							 const Vector2& pos) {
		if (button == MouseButton::Right) {
			dragMove = false;
		}
	}
	
	void MainView::OnMouseWheel(const Vector2& pos) {
		if (GetManager().GetKeyModifierState(KeyModifier::Control))
			editor.Strafe(pos * -.05f);
		else if (GetManager().GetKeyModifierState(KeyModifier::Shift))
			editor.SideMove(pos * -.05f);
		else
			editor.Turn(pos * .02f * Vector2(1, -1));
	}
	
} }

