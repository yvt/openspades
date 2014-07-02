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

#include <Core/RefCountedObject.h>
#include <Client/IRenderer.h>
#include <Client/IAudioDevice.h>
#include <Gui/View.h>

namespace spades { namespace editor {
	
	class UIElement;
	class UIManager;
	class MainView;
	
	class Editor: public gui::View {
		Handle<client::IRenderer> renderer;
		Handle<client::IAudioDevice> audio;
		
		Handle<UIManager> ui;
		Handle<MainView> mainView;
		
		Vector3 viewCenter;
		Vector3 viewAngle;
		float viewDistance;
		
		client::SceneDefinition CreateSceneDefinition();
		client::SceneDefinition sceneDef;
		
	protected:
		~Editor();
	public:
		
		Editor(client::IRenderer *,
			   client::IAudioDevice *);
		
		client::IRenderer *GetRenderer() { return renderer; }
		client::IAudioDevice *GetAudioDevice() { return audio; }
		
		void Turn(const Vector2&);
		void Strafe(const Vector2&);
		void SideMove(const Vector2&);
		
		/*---- implementations of gui::View ----*/
		void MouseEvent(float x, float y) override;
		void KeyEvent(const std::string&,
					  bool down) override;
		void TextInputEvent(const std::string&) override;
		void TextEditingEvent(const std::string&,
							  int start, int len) override;
		bool AcceptsTextInput() override;
		AABB2 GetTextInputRect() override;
		bool NeedsAbsoluteMouseCoordinate() override;
		void WheelEvent(float x, float y) override;
		
		void RunFrame(float dt) override;
		
		void Closing() override;
		
		bool WantsToBeClosed() override;
		
		
		
		
	};
	
} }
