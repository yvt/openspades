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

#include "Host.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include "Shared.h"

SPADES_SETTING(sv_bandwidth, "0");
SPADES_SETTING(sv_maxClients, "64");

namespace spades { namespace server {
	
	static unsigned int nextPeerId;
	struct PeerInfo {
		unsigned int uniqueId;
		void *data;
		
		PeerInfo():uniqueId(nextPeerId++) {
			
		}
		
	};
	
	Host::Host(HostListener *listener):
	host(nullptr),
	numPeers(0),
	listener(listener){
		SPADES_MARK_FUNCTION();
		
		enet_initialize();
		SPLog("ENet initialized");
		
		host = enet_host_create(NULL,
								256, 1,
								(int)sv_bandwidth,
								(int)sv_bandwidth);
		SPLog("ENet host created");
		if(!host){
			SPRaise("Failed to create ENet host");
		}
		
		if(enet_host_compress_with_range_coder(host) < 0)
			SPRaise("Failed to enable ENet Range coder.");
		
		SPLog("ENet Range Coder Enabled");
	}
	
	Host::~Host() {
		// TODO: ensure ClientDisconnected is called
		if(host) enet_host_destroy(host);
		SPLog("ENet host destroyed");
	}
	
	static void LogPeerInfo(ENetPeer *peer) {
		const auto& addr = peer->address;
		char buf[256];
		auto *pinfo = reinterpret_cast<PeerInfo *>(peer->data);
		if(enet_address_get_host_ip(&addr, buf, 256) < 0) {
			sprintf(buf, "%x:%u", addr.host, addr.port);
		}
		SPLog("[Client#%8u] New client connected from %s",
			  pinfo->uniqueId, buf);
		
	}
	
	void Host::DoEvents() {
		ENetEvent event;
		PeerInfo *pinfo;
		
		while(enet_host_service(host, &event, 0) > 0) {
			switch(event.type) {
				case ENET_EVENT_TYPE_CONNECT:
					if(numPeers >= (int)sv_maxClients) {
						enet_peer_disconnect(event.peer, static_cast<uint32_t>(protocol::DisconnectReason::ServerFull));
						break;
					}
					
					pinfo = new PeerInfo();
					event.peer->data = reinterpret_cast<void*>(pinfo);
					LogPeerInfo(event.peer);
					
					try {
						pinfo->data = listener->ClientConnected(pinfo->uniqueId);
					} catch (const std::exception& ex) {
						SPLog("[Client#%8u] Rejecting because of an error: %s", pinfo->uniqueId, ex.what());
						delete pinfo;
						event.peer->data = nullptr;
						enet_peer_disconnect(event.peer, static_cast<uint32_t>(protocol::DisconnectReason::InternalServerError));
					}
					
					break;
					
				case ENET_EVENT_TYPE_RECEIVE:
					// TODO
					break;
			
				case ENET_EVENT_TYPE_DISCONNECT:
					pinfo = reinterpret_cast<PeerInfo *>(event.peer->data);
					if(!pinfo) {
						break;
					}
					
					SPLog("[Client#%8u] Disconnected", pinfo->uniqueId);
					
					try {
						listener->ClientDisconnected(pinfo->data);
					} catch (const std::exception& ex) {
						SPLog("[Client#%8u] Erro while disconnection: %s", pinfo->uniqueId, ex.what());
					}
					
					delete pinfo;
					event.peer->data = nullptr;
					break;
				case ENET_EVENT_TYPE_NONE:
					break;
			}
		}
	}
	
} }

