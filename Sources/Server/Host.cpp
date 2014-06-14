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
#include <Core/Stopwatch.h>
#include <cstring>
#include <Core/ENetTools.h>

SPADES_SETTING(sv_bandwidth, "0");
SPADES_SETTING(sv_maxClients, "64");
SPADES_SETTING(sv_ignoreErrors, "1");

namespace spades { namespace server {
	
	Host::Host(HostListener *listener):
	host(nullptr),
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
		SPADES_MARK_FUNCTION();
		TearDown();
	}
	
	void Host::TearDown() {
		SPADES_MARK_FUNCTION();
		
		if(host){
			// disconnect all peers
			for(const auto& conn: connections) {
				enet_peer_disconnect_later(conn.first, static_cast<uint32_t>(protocol::DisconnectReason::ServerStopped));
			}
			
			// wait for all clients to disconnect gracefully
			Stopwatch sw;
			ENetEvent event;
			while((!connections.empty()) &&
				  sw.GetTime() < 3.0) {
				int ret = enet_host_service(host, &event, 100);
				if(ret > 0) {
					switch(event.type) {
						case ENET_EVENT_TYPE_CONNECT:
							// we don't accept more players
							enet_peer_disconnect_later(event.peer, static_cast<uint32_t>(protocol::DisconnectReason::ServerStopped));
							break;
						case ENET_EVENT_TYPE_DISCONNECT:
							HandleDisconnect(event);
							break;
						default:
							break;
					}
				}else if(ret < 0) {
					SPLog("ENet returned error while servicing");
					break;
				}else{
					// timeout of 100ms
				}
			}
			
			enet_host_destroy(host);
			SPLog("ENet host destroyed");
			host = nullptr;
		}
	}
	
	static void LogPeerInfo(ENetPeer *peer) {
		SPADES_MARK_FUNCTION();
		const auto& addr = peer->address;
		char buf[256];
		auto *pinfo = reinterpret_cast<std::weak_ptr<HostPeer> *>(peer->data);
		if(enet_address_get_host_ip(&addr, buf, 256) < 0) {
			sprintf(buf, "%x:%u", addr.host, addr.port);
		}
		
		auto hpeer = pinfo->lock();
		if(hpeer)
			SPLog("[Client#%8u] New client connected from %s",
				  hpeer->GetUniqueId(), buf);
		else
			SPLog("[Client????????] New client connected from %s",
				  buf);
		
	}
	
	void Host::Broadcast(const protocol::Packet &packet) {
		auto data = packet.Generate();
		auto *p = enet_packet_create(data.data(), data.size(),
									 ENET_PACKET_FLAG_NO_ALLOCATE);
		enet_host_broadcast(host, 0, p);
		enet_packet_destroy(p);	}
	
	void Host::DoEvents() {
		SPADES_MARK_FUNCTION();
		ENetEvent event;
		
		while(enet_host_service(host, &event, 0) > 0) {
			switch(event.type) {
				case ENET_EVENT_TYPE_CONNECT:
					HandleConnect(event);
					break;
					
				case ENET_EVENT_TYPE_RECEIVE:
					HandleReceive(event);
					break;
			
				case ENET_EVENT_TYPE_DISCONNECT:
					HandleDisconnect(event);
					break;
				case ENET_EVENT_TYPE_NONE:
					break;
			}
		}
	}
	
	void Host::HandleConnect(ENetEvent& event) {
		SPADES_MARK_FUNCTION();
		auto *peer = event.peer;
		
		if(connections.size() >= (int)sv_maxClients) {
			enet_peer_disconnect(peer, static_cast<uint32_t>(protocol::DisconnectReason::ServerFull));
			return;
		}
		
		
		unsigned int uniqueId;
		uniqueId = 0;
		try {
			std::shared_ptr<HostPeer> hpeer(new HostPeer(this, event.peer));
			uniqueId = hpeer->GetUniqueId();
			peer->data = reinterpret_cast<void*>(new std::weak_ptr<HostPeer>(hpeer));
			
			LogPeerInfo(event.peer);
			
			listener->ClientConnected(hpeer.get());
			
			connections[peer] = hpeer;
		} catch (const std::exception& ex) {
			SPLog("[Client#%8u] Rejecting because of an error: %s", uniqueId, ex.what());
			peer->data = nullptr;
			enet_peer_disconnect(peer, static_cast<uint32_t>(protocol::DisconnectReason::InternalServerError));
		}
	}
	
	void Host::HandleDisconnect(ENetEvent& event) {
		SPADES_MARK_FUNCTION();
		auto *weakHPeer = reinterpret_cast<std::weak_ptr<HostPeer> *>(event.peer->data);
		if(!weakHPeer) {
			return;
		}
		
		auto hpeer = weakHPeer->lock();
		if(hpeer) {
			SPLog("[Client#%8u] Disconnected", hpeer->GetUniqueId());
			
			auto it = connections.find(event.peer);
			if(it != connections.end())
				connections.erase(it);
			
			try {
				hpeer->OnDisconnected();
			} catch (const std::exception& ex) {
				SPLog("[Client#%8u] Error while disconnection: %s", hpeer->GetUniqueId(), ex.what());
			}
			
		}
		delete weakHPeer;
		event.peer->data = nullptr;
	}
	
	void Host::HandleReceive(ENetEvent &event){
		SPADES_MARK_FUNCTION();
		auto *weakHPeer = reinterpret_cast<std::weak_ptr<HostPeer> *>(event.peer->data);
		if(!weakHPeer) {
			return;
		}
		
		auto hpeer = weakHPeer->lock();
		if(!hpeer) return;
		
		ENetPacket *data = event.packet;
		auto& buffer = packetBuffer;
		buffer.resize(data->dataLength);
		std::memcpy(buffer.data(), data->data, buffer.size());
		
		try {
			
			std::unique_ptr<protocol::Packet> packet;
			packet.reset(protocol::Packet::Decode(buffer));
			
			if(!packet) {
				SPRaise("Packet couldn't be decoded.");
			}
			
			hpeer->OnReceived(*packet);
		} catch (const std::exception& ex) {
			if(!sv_ignoreErrors) {
				SPLog("[Client#%8u] Incoming packet processing error: %s",
					  hpeer->GetUniqueId(), ex.what());
				NetPacketReader reader(buffer);
				reader.DumpDebug();
			}
		}
		
	}
	
#pragma mark - HostPeer
	
	static unsigned int nextPeerId = 0;
	
	HostPeer::HostPeer(Host *host, ENetPeer *enetPeer):
	uniqueId(nextPeerId++),
	host(host),
	enetPeer(enetPeer),
	disconnectNotified(false) {
		char buf[24];
		sprintf(buf, "[Client#%8u]", uniqueId);
		logHeader = buf;
	}
	
	HostPeer::~HostPeer(){
		OnDisconnected();
	}
	
	void HostPeer::SetListener(HostPeerListener *listener) {
		this->listener.Set(listener, true);
	}
	
	void HostPeer::Send(const protocol::Packet &packet) {
		auto data = packet.Generate();
		auto *p = enet_packet_create(data.data(), data.size(),
										  ENET_PACKET_FLAG_NO_ALLOCATE);
		enet_peer_send(enetPeer, 0, p);
		enet_packet_destroy(p);
	}
	
	void HostPeer::Disconnect(protocol::DisconnectReason reason) {
		enet_peer_disconnect_later(enetPeer, static_cast<uint32_t>(reason));
	}
	
	void HostPeer::OnDisconnected() {
		if(!disconnectNotified) {
			auto listener = this->listener;
			if(listener) listener->Disconnected();
			disconnectNotified = true;
		}
	}
	
	void HostPeer::OnReceived(const protocol::Packet &packet) {
		auto listener = this->listener;
		if(listener) listener->PacketReceived(packet);
	}
	
} }

