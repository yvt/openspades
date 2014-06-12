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

#include <Gui/View.h>
#include <Client/IRenderer.h>
#include <Client/IAudioDevice.h>

namespace spades { namespace game {
	class World;
} }

namespace spades { namespace ngclient {
	
	class Client;
	
	enum class CameraMode {
		
	};
	
	class ArenaCamera {
	public:
		virtual ~ArenaCamera() { }
		virtual client::SceneDefinition CreateSceneDefinition
		(client::IRenderer&) = 0;
	};
	
	class Arena: public RefCountedObject
	{
		Handle<Client> client;
		Handle<client::IRenderer> renderer;
		Handle<client::IAudioDevice> audio;
		Handle<game::World> world;
		
		bool setupped = false;
		
		ArenaCamera& GetCamera();
		class DefaultCamera: public ArenaCamera {
			Arena& arena;
		public:
			DefaultCamera(Arena& a): arena(a) { }
			virtual client::SceneDefinition CreateSceneDefinition
			(client::IRenderer&);
		};
		DefaultCamera defaultCamera { *this };
		
		void Render();
		client::SceneDefinition CreateSceneDefinition();
		
	public:
		
		Arena(Client *);
		
		~Arena();
		
		/*---- implementations of gui::View ---- */
		void MouseEvent(float x, float y);
		void KeyEvent(const std::string&,
					  bool down);
		void TextInputEvent(const std::string&);
		void TextEditingEvent(const std::string&,
							  int start, int len);
		bool AcceptsTextInput();
		AABB2 GetTextInputRect();
		bool NeedsAbsoluteMouseCoordinate();
		void WheelEvent(float x, float y);
		
		void RunFrame(float dt); // Arena.cpp
		
		void SetupRenderer(); // Arena.cpp
		void UnsetupRenderer();
		
		bool WantsToBeClosed(); // Arena.cpp
		
	
		
		
	};
	
} }
