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
#include <Server/Shared.h>
#include <Core/Thread.h>
#include <Core/Mutex.h>
#include <Core/AutoLocker.h>
#include <Core/Stopwatch.h>
#include <memory>
#include <set>
#include <functional>
#include <list>
#include <Core/ServerAddress.h>

namespace spades { namespace ngclient {
	
	class HostListener;
	
	class Host {
		
		std::set<HostListener*> listeners;
		
		/** Client becomes unresponsive sometimes (e.g.
		 * late compilation of GLSL shaders by video drivers).
		 * On some environments, this takes so long that
		 * the client gets disconnected due to time-out. */
		class AntiTimeoutTimer;
		std::unique_ptr<AntiTimeoutTimer> antiTimeoutTimer;
		Stopwatch antiTimeoutStopwatch;
		std::list<std::function<void()>> eventQueue;
		
		/** access to ENet structure must be synchronized */
		Mutex mutex;
		
		ENetHost *host = nullptr;
		ENetPeer *peer = nullptr;
		
		void Service();
		
		void HandleConnect();
		void HandleDisconnect(protocol::DisconnectReason);
		void HandlePacket(protocol::Packet&);
		
	public:
		
		Host();
		~Host();
		
		void AddListener(HostListener *);
		void RemoveListener(HostListener *);
		
		void TearDown();
		
		void Connect(const ServerAddress&);
		void Disconnect();
		void DoEvents();
		bool Send(protocol::Packet&);
		
	};
	
	class HostListener {
		friend class Host;
	protected:
		virtual void ConnectedToServer() { }
		
		virtual void DisconnectedFromServer(protocol::DisconnectReason) { }
		
		virtual void PacketReceived(protocol::Packet&) { }
	public:
		virtual ~HostListener() { }
	};
	
} }
