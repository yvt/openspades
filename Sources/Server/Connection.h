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
#include <list>
#include "Shared.h"
#include <memory>

namespace spades { namespace game {
	class World;
	class Player;
} }

namespace spades { namespace server {
	
	class Server;
	
	class Connection: public HostPeerListener {
		friend class Server;
		
		class PacketVisitor;
		class MapGenerator;
		class EntityUpdater;
		
		enum class State {
			/** Waiting for InitiateConnectionPacket. */
			NotInitiated,
			
			/** Waiting for ClientCertificatePacket. */
			WaitingForCertificate,
			MapTransfer,
			CompletingMapTransfer,
			Game
		};
		
		Server *server;
		HostPeer *peer;
		State state = State::NotInitiated;
		double stateTimeout = 10.0;
		std::string serverNonce;
		std::string clientNonce;
		std::string nonce;
		int mapQuality;
		std::string playerName;
		
		MapGenerator *mapGenerator;
		
		Handle<game::Player> player;
		bool localPlayerNotified = false;
		
		game::World& GetWorld();
		
		void SendServerCertificate();
		void StartStateTransfer();
		void FinalStateTransfer();
		void Join();
		void Leave();
		void HandleGenericCommand(const std::vector<std::string>&);
		
	protected:
		// RefCountedObject should only be destroyed by
		// RefCountedObject::Release().
		// (not delete expr nor std::shared_ptr)
		virtual ~Connection();
	private:
		void Initialize(HostPeer *);
	public:
		Connection(Server *);
		
		void Kick(const std::string& reason);
		
		void Update(double dt);
		
		/** Responds to the world change. */
		void OnWorldChanged();
		
		/* ---- HostPeerListener ----*/
		void PacketReceived(const protocol::Packet&) override;
		void Disconnected() override;
	};
	
} }
