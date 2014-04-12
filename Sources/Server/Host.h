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
#include <ENet/enet.h>

namespace spades { namespace server {
	
	class HostListener {
	public:
		virtual ~HostListener() {}
		
		/** Called when an new client connected to the server.
		 * Do not throw exception to reject the client because
		 * the client cannot know the reason. */
		virtual void *ClientConnected(unsigned int uniqueId) = 0;
		
		/** Called when a client has disconnected from the server. */
		virtual void ClientDisconnected(void *) = 0;
	};
	
	class Host {
		ENetHost *host;
		int numPeers;
		HostListener *listener;
	public:
		Host(HostListener *);
		~Host();
		void DoEvents();
	};
	
} }

