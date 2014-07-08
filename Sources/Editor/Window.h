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
	
	class Window: public UIElement {
		class Mover;
		class DragHandle;
		class DragHandles;
		class CloseButton;
		
		Handle<Mover> moverGrab;
		Handle<Mover> moverTop;
		Handle<Mover> moverBottom;
		Handle<Mover> moverLeft;
		Handle<Mover> moverRight;
		Handle<Mover> moverTopLeft;
		Handle<Mover> moverTopRight;
		Handle<Mover> moverBottomLeft;
		Handle<Mover> moverBottomRight;
		
		Handle<CloseButton> closeButton;
		
		void Layout();
		
	protected:
		~Window();
		void RenderClient() override;
		
		virtual client::IFont *GetTitleFont();
		virtual std::string GetTitle();
		virtual void OnClose();
		
	public:
		Window(UIManager *);
		
		AABB2 GetClientBounds();
		virtual Vector2 AdjustClientSize(const Vector2&);
		Vector2 AdjustSize(const Vector2&);
	};

} }
