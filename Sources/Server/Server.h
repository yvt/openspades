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
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <Core/RefCountedObject.h>
#include <Game/World.h>

namespace spades { namespace game {
	class World;
	class Entity;
} }

namespace spades { namespace server {
	
	class Connection;
	class ServerEntity;
	class ServerPlayer;
	
	class Server:
	public HostListener,
	game::WorldListener {
		friend class Connection;
		
		std::unique_ptr<Host> host;
		std::unordered_set<Connection *> connections;
		
		Handle<game::World> world;
		
		std::list<std::unique_ptr<ServerEntity>>
		serverEntities;
		std::unordered_map<game::Entity *,
		std::list<std::unique_ptr<ServerEntity>>::iterator>
		entityToServerEntity;
		void AddServerEntity(ServerEntity *);
		ServerEntity *GetServerEntityForEntity(game::Entity*);
		
		std::list<std::unique_ptr<ServerPlayer>>
		serverPlayers;
		std::unordered_map<game::Player *,
		std::list<std::unique_ptr<ServerPlayer>>::iterator>
		playerToServerPlayer;
		void AddServerPlayer(ServerPlayer *);
		ServerPlayer *GetServerPlayerForPlayer(game::Player*);
		
		void SetWorld(game::World *);
		
		/* ---- WorldListener ---- */
		void EntityLinked(game::World&, game::Entity *) override;
		void EntityUnlinked(game::World&, game::Entity *) override;
		
		void PlayerCreated(game::World&, game::Player *) override;
		void PlayerRemoved(game::World&, game::Player *) override;
		
		void FlushMapEdits(const std::vector<game::MapEdit>&) override;
		
	public:
		Server();
		virtual ~Server();
		
		void Update(double dt);
		
		game::World& GetWorld() { return *world; }
		
		void ClientConnected(HostPeer *peer) override;
		
		
	};

} }

