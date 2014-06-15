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

#include "Client.h"

#include "Arena.h"
#include <Game/World.h>

#include <Server/Server.h>
#include "NetworkClient.h"
#include <Core/Settings.h>

SPADES_SETTING(cg_playerName, "");

namespace spades { namespace ngclient {
	
	Client::Client(client::IRenderer *renderer,
				   client::IAudioDevice *audio,
				   const ClientParams& params):
	renderer(renderer),
	audio(audio),
	params(params) {
		SPAssert(renderer);
		SPAssert(audio);
		
		renderer->SetGameMap(nullptr);
		
		if (params.hostLocalServer) {
			server.reset(new server::Server());
		}
		
		NetworkClientParams netParams;
		netParams.address = ServerAddress
		(params.host, ProtocolVersion::Ngspades);
		if (params.hostLocalServer) {
			netParams.address = ServerAddress
			("127.0.0.1:2064", ProtocolVersion::Ngspades);
		}
		netParams.playerName = (std::string)cg_playerName;
		net.reset(new NetworkClient(netParams));
		net->AddListener(this);
		
		net->Connect();
		
		//arena.Set(new Arena(this), false);
		//arena->SetupRenderer();
	}
	
	Client::~Client() {
		if(arena)
		   arena->UnsetupRenderer();
		arena.Set(nullptr);
		net->RemoveListener(this);
	}
	
	void Client::RunFrame(float dt) {
		if (net) {
			net->Update();
		}
		if (server) {
			server->Update(dt);
		}
		
		if (arena) {
			arena->RunFrame(dt);
		}
		
		renderer->Flip();
	}
	
	void Client::Closing() {
		arena.Set(nullptr);
	}
	
	bool Client::WantsToBeClosed() {
		return false;
	}
	
	Client::InputRoute Client::GetInputRoute() {
		return arena ? InputRoute::Arena : InputRoute::None;
	}
	
#pragma mark - NetworkClientListener
	
	void Client::WorldChanged(game::World *w) {
		if (w) {
			arena.Set(new Arena(this, w), false);
			arena->SetupRenderer();
		} else {
			if(arena)
				arena->UnsetupRenderer();
			arena.Set(nullptr);
		}
	}
	
	void Client::Disconnected(const std::string &reason) {
		SPLog("Disconnected: %s", reason.c_str());
	}
	
} }

