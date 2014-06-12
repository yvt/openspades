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

namespace spades { namespace ngclient {
	
	struct ClientParams {
		std::string host;
		bool hostLocalServer;
	};
	
	class Arena;
	
	class Client: public gui::View
	{
		friend class Arena;
		Handle<client::IRenderer> renderer;
		Handle<client::IAudioDevice> audio;
		ClientParams const params;
		
		Handle<Arena> arena;
		enum class InputRoute
		{
			Arena,
			ClientUI
		};
		InputRoute GetInputRoute();
		
	public:
		
		Client(client::IRenderer *,
			   client::IAudioDevice *,
			   const ClientParams&);
		
		~Client();
		
		/*---- implementations of gui::View ----
		 * most of them are implemented in Client_Input.cpp. */
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
		
		void RunFrame(float dt) override; // Client.cpp
		
		void Closing() override; // Client.cpp
		
		bool WantsToBeClosed() override; // Client.cpp
		
		
	};
	
} }
