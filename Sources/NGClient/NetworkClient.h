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

#include <memory>
#include "Host.h"
#include <set>
#include <Core/RefCountedObject.h>
#include <Core/ServerAddress.h>
#include <vector>
#include <list>
#include <Core/TMPUtils.h>

namespace spades { namespace game {
	class World;
} }
	
namespace spades { namespace ngclient {
	
	class Host;
	
	struct NetworkClientParams {
		std::string playerName;
		ServerAddress address;
	};
	
	class NetworkClientListener;
	
	class NetworkClient: HostListener
	{
		class PacketVisitor;
		class MapLoader;
		class EntityUpdater;
		
		enum class State {
			NotConnected,
			WaitingForGreeting,
			WaitingForServerCertificate,
			WaitingForGameState,
			LoadingMap,
			LoadingGameState,
			Game
		};
		
		std::set<NetworkClientListener *> listeners;
		
		std::unique_ptr<Host> host;
		State state = State::NotConnected;
		std::string nonce;
		
		std::string progressMessage;
		stmp::optional<float> progress;
		
		std::unique_ptr<MapLoader> mapLoader;
		std::list<std::vector<game::MapEdit>> savedMapEdits;
		
		Handle<game::World> world;
		
		NetworkClientParams params;
		
		void SendInitiateConnection(const std::string& serverNonce);
		void SendClientCertificate();
		void Kicked(const std::string&);
		void HandleGenericCommand(const std::vector<std::string>&);
		
		void SetWorld(game::World *);
		
		/* ---- HostListener ---- */
		void ConnectedToServer() override;
		
		void DisconnectedFromServer(protocol::DisconnectReason) override;
		
		void PacketReceived(protocol::Packet&) override;
		
	public:
		NetworkClient(const NetworkClientParams&);
		~NetworkClient();
		
		void AddListener(NetworkClientListener *);
		void RemoveListener(NetworkClientListener *);
		
		void Connect();
		
		stmp::optional<float> GetProgress() { return progress; }
		std::string GetProgressMessage() { return progressMessage; }
		
		void SendGenericCommand(const std::vector<std::string>&);
		
		void Update();
	};
	
	class NetworkClientListener {
		friend class NetworkClient;
	protected:
		virtual void WorldChanged(game::World *) { }
		virtual void Disconnected(const std::string& reason) { }
	public:
		virtual ~NetworkClientListener() { }
	};
	
} }
