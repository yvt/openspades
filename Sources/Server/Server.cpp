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

#include "Server.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Exception.h>
#include "Connection.h"
#include <Game/World.h>
#include "ServerEntity.h"
#include "PlayerServerEntity.h"
#include "Shared.h"
#include <Core/IStream.h>
#include <Core/FileManager.h>

#include <Game/AllEntities.h>

namespace spades { namespace server {
	
	Server::Server() {
		SPLog("Server starting.");
		host.reset(new Host(this));
		
		// create initial world
		game::WorldParameters params;
		
		// TODO: provide correct map
		std::unique_ptr<IStream> stream(FileManager::OpenForReading("Maps/isle7.vxl"));
		Handle<client::GameMap> map(client::GameMap::Load(stream.get()), false);
		Handle<game::World> w(new game::World(params, map), false);
		SetWorld(w);
	}
	
	Server::~Server() {
		SPLog("Server closing.");
		host->TearDown();
		host.reset();
	}
	
	void Server::ClientConnected(HostPeer *peer) {
		Handle<Connection> conn(new Connection(this), false);
		conn->Initialize(peer);
		connections.insert(&*conn);
	}
	
	void Server::SetWorld(game::World *world) {
		if (this->world) {
			this->world->RemoveListener(this);
		}
		serverEntities.clear();
		entityToServerEntity.clear();
		this->world.Set(world, true);
		if (world)
			world->AddListener(this);
		for (auto *conn: connections) {
			conn->OnWorldChanged();
		}
	}
	
	void Server::Update(double dt) {
		// network event handling
		host->DoEvents();
		
		// process every connections
		for (auto *conn: connections)
			conn->Update(dt);
		
		// update world
		world->Advance(dt);
		for (const auto& e: serverEntities) {
			e->Update(dt);
		}
		
		// send entity update
		protocol::EntityUpdatePacket packet;
		for (const auto& e: serverEntities) {
			auto delta = e->DeltaSerialize();
			if (delta) {
				packet.items.push_back(*delta);
			}
		}
		if (!packet.items.empty()) {
			host->Broadcast(packet);
		}
		
	}
	
	void Server::AddServerEntity(ServerEntity *e) {
		SPAssert(e);
		SPAssert(&e->GetServer() == this);
		serverEntities.emplace_front(e);
		entityToServerEntity[&e->GetEntity()] = serverEntities.begin();
 	}
	
	ServerEntity *Server::GetServerEntityForEntity(game::Entity *e) {
		auto it = entityToServerEntity.find(e);
		if (it == entityToServerEntity.end()) return nullptr;
		return it->second->get();
	}
	
#pragma mark - WorldListener
	void Server::EntityLinked(game::World &, game::Entity *e) {
		struct Factory: public game::EntityVisitor {
			Server& server;
			Factory(Server& server): server(server) { }
			
			void Visit(game::PlayerEntity& e) override {
				server.AddServerEntity(new PlayerServerEntity(e, server));
			}
			void Visit(game::GrenadeEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
			void Visit(game::RocketEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
			void Visit(game::CommandPostEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
			void Visit(game::FlagEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
			void Visit(game::CheckpointEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
			void Visit(game::VehicleEntity& e) override {
				server.AddServerEntity(new ServerEntity(e, server));
			}
		};
		Factory f(*this);
		e->Accept(f);
	}
	
	void Server::EntityUnlinked(game::World &, game::Entity *e) {
		protocol::EntityRemovePacket packet;
		packet.entityId = *e->GetId();
		host->Broadcast(packet);
		
		auto it = entityToServerEntity.find(e);
		SPAssert(it != entityToServerEntity.end());
		auto it2 = it->second;
		entityToServerEntity.erase(it);
		serverEntities.erase(it2);
	}
	
	void Server::FlushMapEdits(const std::vector<game::MapEdit> &edits) {
		protocol::TerrainUpdatePacket packet;
		packet.edits = edits;
		host->Broadcast(packet);
	}
} }

