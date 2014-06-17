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
#include "NetworkClient.h"

namespace spades { namespace client {
	class IFont;
} }
namespace spades { namespace server {
	class Server;
} }

namespace spades { namespace ngclient {
	
	struct ClientParams {
		std::string host;
		bool hostLocalServer;
	};
	
	class Arena;
	
	class NetworkClient;
	
	class Client:
	public gui::View,
	NetworkClientListener
	{
		friend class Arena;
		double time = 0.0;
		Handle<client::IRenderer> renderer;
		Handle<client::IAudioDevice> audio;
		std::unique_ptr<server::Server> server;
		std::unique_ptr<NetworkClient> net;
		ClientParams const params;
		
		Handle<client::IFont> font;
		
		Handle<Arena> arena;
		enum class InputRoute
		{
			None,
			Arena,
			ClientUI
		};
		InputRoute GetInputRoute();
		
		void WorldChanged(game::World *) override;
		void Disconnected(const std::string& reason) override;
		
		void RenderLoadingScreen(float dt);
		
	public:
		
		Client(client::IRenderer *,
			   client::IAudioDevice *,
			   const ClientParams&);
		
		~Client();
		
		bool IsHostingServer() { return server != nullptr; }
		
		client::IRenderer *GetRenderer() { return renderer; }
		
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
