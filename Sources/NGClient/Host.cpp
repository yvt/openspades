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
#include <Core/Settings.h>
#include <thread>

SPADES_SETTING(cl_bandwidth, "0");

namespace spades { namespace ngclient {
	
	class Host::AntiTimeoutTimer:
	public Thread
	{
		Host& host;
		bool volatile stopNow = false;
	public:
		AntiTimeoutTimer(Host& host): host(host) { }
		void Run() override {
			SPADES_MARK_FUNCTION();
			while (!stopNow) {
				{
					AutoLocker lock(&host.mutex);
					if (host.antiTimeoutStopwatch.GetTime() > .5) {
						
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		
		void Stop() {
			stopNow = true;
			Join();
		}
	};
	
	Host::Host() {
		SPADES_MARK_FUNCTION();
		
		enet_initialize();
		SPLog("ENet initialized");
		
		host = enet_host_create(NULL,
								2, 1,
								(int)cl_bandwidth,
								(int)cl_bandwidth);
		SPLog("ENet host created");
		if(!host){
			SPRaise("Failed to create ENet host");
		}
		
		if(enet_host_compress_with_range_coder(host) < 0)
			SPRaise("Failed to enable ENet Range coder.");
		
		SPLog("ENet Range Coder Enabled");
	
		antiTimeoutTimer.reset(new AntiTimeoutTimer(*this));
		SPLog("Anti-timeout timer Enabled");
	}
	
	Host::~Host() {
		SPADES_MARK_FUNCTION();
		TearDown();
	}
	
	void Host::AddListener(HostListener *l) {
		SPAssert(l);
		listeners.insert(l);
	}
	
	void Host::RemoveListener(HostListener *l) {
		listeners.erase(l);
	}
	
	void Host::DoEvents() {
		{
			AutoLocker lock(&mutex);
			antiTimeoutStopwatch.Reset();
		}
		
		Service();
		
		while (true) {
			std::function<void()> event;
			
			{
				AutoLocker lock(&mutex);
				if (eventQueue.empty()) return;
				event = std::move(eventQueue.front());
				eventQueue.pop_front();
			}
			
			event();
		}
	}
	
	void Host::Service() {
		AutoLocker lock(&mutex);
		
		if(!host) return;
		
		ENetEvent event;
		while(enet_host_service(host, &event, 0) > 0){
			if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
				auto reason = static_cast<protocol::DisconnectReason>(event.data);
				eventQueue.push_back([=]() {
					HandleDisconnect(reason);
				});
			} else if (event.type == ENET_EVENT_TYPE_CONNECT) {
				eventQueue.push_back([=]() {
					HandleConnect();
				});
			} else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
				auto *pdata = event.packet;
				std::vector<char> data;
				data.resize(pdata->dataLength);
				memcpy(data.data(), pdata->data, pdata->dataLength);
				enet_packet_destroy(pdata);
				std::shared_ptr<protocol::Packet> packet
				(protocol::Packet::Decode(std::move(data)));
				eventQueue.push_back([=]() {
					HandlePacket(*packet);
				});
			}
		}
	}
	
	void Host::HandleConnect() {
		for (auto *l: listeners)
			l->ConnectedToServer();
	}
	
	void Host::HandleDisconnect(protocol::DisconnectReason reason) {
		for (auto *l: listeners)
			l->DisconnectedFromServer(reason);
	}
	
	void Host::HandlePacket(protocol::Packet &p) {
		for (auto *l: listeners)
			l->PacketReceived(p);
	}
	
	bool Host::Send(protocol::Packet &packet) {
		SPADES_MARK_FUNCTION();
		
		AutoLocker lock(&mutex);
		if (!host) return false;
		
		auto data = packet.Generate();
		auto *p = enet_packet_create(data.data(), data.size(),
									 ENET_PACKET_FLAG_NO_ALLOCATE);
		enet_peer_send(peer, 0, p);
		enet_packet_destroy(p);
		
		return true;
	}
	
	void Host::TearDown() {
		SPADES_MARK_FUNCTION();
		
		if (antiTimeoutTimer) {
			antiTimeoutTimer->Stop();
			antiTimeoutTimer.reset();
		}
		
		if(host){
			if (peer) {
				enet_peer_disconnect(peer, 0);
			}
			
			// wait for all clients to disconnect gracefully
			Stopwatch sw;
			ENetEvent event;
			
			while(sw.GetTime() < 1.0 && peer) {
				int ret = enet_host_service(host, &event, 100);
				if(ret > 0) {
					switch(event.type) {
						case ENET_EVENT_TYPE_DISCONNECT:
							peer = nullptr;
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
			
			if (peer) {
				// timed out
				enet_peer_reset(peer);
			}
			
			enet_host_destroy(host);
			SPLog("ENet host destroyed");
			host = nullptr;
		}
	}
	
} }

