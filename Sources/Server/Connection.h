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

#include "Host.h"

namespace spades { namespace server {
	
	class Server;
	
	class Connection: public HostPeerListener {
		friend class Server;
		
		Server *server;
		HostPeer *peer;
	protected:
		// RefCountedObject should only be destroyed by
		// RefCountedObject::Release().
		// (not delete expr nor std::shared_ptr)
		virtual ~Connection();
	private:
		void Initialize(HostPeer *);
	public:
		Connection(Server *);
		
		virtual void PacketReceived(const protocol::Packet&);
		virtual void Disconnected();
	};
	
} }
