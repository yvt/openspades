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

#include <Game/Player.h>
#include "Shared.h"

namespace spades { namespace server {
	class Server;
	
	class ServerPlayer {
		Server& server;
		game::Player& player;
		protocol::PlayerUpdateItem lastState;
	public:
		ServerPlayer(game::Player&, Server&);
		virtual ~ServerPlayer();
		
		virtual void Update(double);
		
		Server& GetServer() { return server; }
		game::Player& GetPlayer() { return player; }
		
		/** Saves the current state for the delta encoding. */
		void SaveForDeltaEncoding();
		
		stmp::optional<protocol::PlayerUpdateItem>
		DeltaSerialize();
		
		virtual protocol::PlayerUpdateItem
		Serialize();
		
	};
} }
