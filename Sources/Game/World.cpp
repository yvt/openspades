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

#include "World.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include "Entity.h"
#include "PlayerEntity.h"

namespace spades { namespace game {
	
	WorldParameters::WorldParameters():
	playerJumpVelocity(0.36f),
	fallDamageVelocity(0.58f),
	fatalFallDamageVelocity(1.f) { }
	
	World::World():
	currentTime(0) {
		// TODO: provide correct map
		std::unique_ptr<IStream> stream(FileManager::OpenForReading("Maps/Title.vxl"));
		gameMap.Set(client::GameMap::Load(stream.get()), false);
	}
	
	World::~World() {
		
	}
	
	void World::AddListener(WorldListener *l) {
		SPAssert(l);
		listeners.insert(l);
	}
	void World::RemoveListener(WorldListener *l) {
		SPAssert(l);
		listeners.erase(l);
	}
	
	Entity *World::FindEntity(uint32_t id) {
		auto it = entities.find(id);
		return it == entities.end() ? nullptr : it->second;
	}
	
	std::vector<Entity *> World::GetAllEntities() {
		std::vector<Entity *> ret;
		for (auto& e: entities) {
			ret.push_back(e.second);
		}
		return ret;
	}
	
	void World::LinkEntity(Entity *e,
						   stmp::optional<uint32_t> entityId) {
		SPAssert(e);
		if (e->GetId()) {
			SPRaise("Entity is already linked to the world.");
		}
		
		if (!entityId) {
			// automatically allocate entity id
			// IDs below 1024 are reserved for players.
			
			if (e->GetType() == EntityType::Player) {
				// player's entity Id must be explicit
				SPRaise("Player's entity ID must be specified explicitly.");
			}
			
			uint32_t last = 1024;
			for (auto it = entities.begin(); it != entities.end(); ++it) {
				if (it->first > last) {
					// found
					entityId = last;
				}
				last = std::max(last, it->first + 1);
			}
			
			// note: allocation failure is almost unlikely because
			// we have 2^32 IDs...
		}
		
		SPAssert(entityId);
		
		entities.emplace(*entityId, Handle<Entity>(e, true));
		e->SetId(entityId);
		
		for (auto *l: listeners)
			l->EntityLinked(*this, e);
	}
	
	void World::UnlinkEntity(Entity *e) {
		SPAssert(e);
		if (!e->GetId()) {
			SPRaise("Entity is not linked to the world.");
		}
		if (&e->GetWorld() != this) {
			SPRaise("Entity is linked to another world.");
		}
		
		Handle<Entity> ee(e);
		
		auto it = entities.find(*e->GetId());
		SPAssert(it != entities.end());
		SPAssert(it->second == e);
		entities.erase(it);
		
		e->SetId(stmp::optional<uint32_t>());
		
		for (auto *l: listeners)
			l->EntityUnlinked(*this, e);
	}
	
	void World::Advance(Duration dt) {
		//SPNotImplemented();
		currentTime += dt;
	}
	
	Vector3 World::GetGravity()	{
		return Vector3(0.f, 0.f, 10.f); // FIXME: correct value
	}
	
	bool World::IsLocalHostServer() {
		return true; // TODO: IsLocalHostServer
	}
	
	bool World::IsLocalHostClient() {
		return true; // TODO: IsLocalHostServer
	}
	
	bool World::IsWaterAt(const Vector3& v) {
		return v.z > 63.f;
	}
	
	stmp::optional<uint32_t> World::GetLocalPlayerId() {
		// TODO: GetLocalPlayerId
		return stmp::optional<uint32_t>();
	}
	
	PlayerEntity *World::GetLocalPlayerEntity() {
		auto id = GetLocalPlayerId();
		if (!id) return nullptr;
		
		auto *e = FindEntity(*id);
		if (!e) return nullptr;
		
		auto *pe = dynamic_cast<PlayerEntity *>(e);
		SPAssert(pe);
		return pe;
	}
	
} }

