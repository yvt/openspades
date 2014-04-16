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
#include <enet/enet.h>
#include "Shared.h"
#include <unordered_map>
#include <Core/RefCountedObject.h>
#include <memory>

namespace spades { namespace server {
	
	class Host;
	
	class HostPeerListener: public RefCountedObject {
	public:
		virtual void PacketReceived(const protocol::Packet&) = 0;
		
		/** Called when a client has disconnected from the server.
		 * After this being called, associated HostPeer is no longer
		 * available. */
		virtual void Disconnected() = 0;
	};
	
	class HostPeer {
		friend class Host;
		unsigned int uniqueId;
		Host *host;
		Handle<HostPeerListener> listener;
		ENetPeer *enetPeer;
		HostPeer(Host *host, ENetPeer *enetPeer);
		bool disconnectNotified;
		
		void OnDisconnected();
		void OnReceived(const protocol::Packet&);
	public:
		~HostPeer();
		
		unsigned int GetUniqueId() { return uniqueId; }
		
		void SetListener(HostPeerListener *listener);
		
		void Send(const protocol::Packet&);
		void Disconnect(protocol::DisconnectReason);
		
	};
	
	class HostListener {
	public:
		virtual ~HostListener() {}
		
		/** Called when an new client connected to the server.
		 * Do not throw exception to reject the client because
		 * the client cannot know the reason.
		 * Exception is only logged to server log. */
		virtual void ClientConnected(HostPeer *peer) = 0;
		
	};
	
	class Host {
		friend class HostPeer;
		
		ENetHost *host;
		HostListener *listener;
		std::unordered_map<ENetPeer *, std::shared_ptr<HostPeer>> connections;
		std::vector<char> packetBuffer;
		
		void HandleConnect(ENetEvent&);
		void HandleDisconnect(ENetEvent&);
		void HandleReceive(ENetEvent&);
		
	public:
		Host(HostListener *);
		~Host();
		void DoEvents();
		void TearDown();
		
		void Broadcast(const protocol::Packet&);
		void BroadcastExcept(const protocol::Packet&, HostPeer *);
	};
	
} }

