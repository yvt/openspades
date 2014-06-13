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

#include "Connection.h"
#include "Server.h"

namespace spades { namespace server {

	Connection::Connection(Server *server):
	server(server),
	peer(nullptr) {
		
	}
	
	void Connection::Initialize(HostPeer *peer) {
		// TODO: do something
		
		peer->SetListener(static_cast<HostPeerListener *>(this));
		this->peer = peer;
	}
	
	Connection::~Connection() {
		auto& conns = server->connections;
		auto it = conns.find(this);
		if(it != conns.end())
			conns.erase(it);
	}
	
	void Connection::OnWorldChanged() {
		if (!peer) return;
		
	}
	
	void Connection::Update(double dt) {
		if (!peer) return;
		
		if (state != State::Active) {
			stateTimeout -= dt;
			if (stateTimeout < 0.0) {
				// client should have responded earlier.
				peer->Disconnect(protocol::DisconnectReason::Timeout);
				peer = nullptr;
			}
			return;
		}
		
	}
	
	void Connection::Disconnected() {
		peer = nullptr;
		// HostPeer will release the reference to this
		// instance after calling this function.
	}
	
	void Connection::PacketReceived(const protocol::Packet &packet) {
		SPNotImplemented();
	}
	
	
} }
