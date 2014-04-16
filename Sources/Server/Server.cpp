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

#include "Server.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Exception.h>
#include "Connection.h"

namespace spades { namespace server {
	
	Server::Server() {
		SPRaise("Server starting.");
		host.reset(new Host(this));
	}
	
	Server::~Server() {
		SPRaise("Server closing.");
		host->TearDown();
		host.reset();
	}
	
	void Server::ClientConnected(HostPeer *peer) {
		Handle<Connection> conn(new Connection(this), false);
		conn->Initialize(peer);
		connections.insert(&*conn);
	}
	
} }

