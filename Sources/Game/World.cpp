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
#include <Client/GameMapWrapper.h>
#include <deque>
#include <unordered_map>
#include <unordered_set>

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
		
		gameMapWrapper.reset(new client::GameMapWrapper(gameMap));
	}
	
	World::~World() {
		gameMapWrapper.release();
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
	
#pragma mark - Map Edits
	
	bool World::IsValidMapCoord(const IntVector3 &pos) {
		return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 &&
		pos.x < gameMap->Width() && pos.y < gameMap->Height() &&
		pos.z < gameMap->Depth();
	}
	
	void World::CreateBlock(const spades::IntVector3& pos, uint32_t color) {
		SPAssert(IsLocalHostServer());
		SPAssert(IsValidMapCoord(pos));
		
		MapEdit edit;
		edit.position = pos;
		edit.color = color;
		
		auto it = mapEdits.find(pos);
		if (it == mapEdits.end()) {
			mapEdits.emplace(pos, edit);
		} else {
			it->second = edit;
		}
	}
	
	void World::DestroyBlock(const spades::IntVector3 &pos) {
		SPAssert(IsLocalHostServer());
		SPAssert(IsValidMapCoord(pos));
		
		MapEdit edit;
		edit.position = pos;
		
		auto it = mapEdits.find(pos);
		if (it == mapEdits.end()) {
			mapEdits.emplace(pos, edit);
		} else {
			it->second = edit;
		}
	}
	
	// FIXME: duplicate code with Client/World.cpp
	static std::vector<std::vector<client::CellPos>> ClusterizeBlocks(const std::vector<client::CellPos>& blocks) {
		using namespace client;
		
		std::unordered_map<CellPos, bool, CellPosHash> blockMap;
		for(const auto& block: blocks) {
			blockMap[block] = true;
		}
		
		std::vector<std::vector<CellPos>> ret;
		std::deque<decltype(blockMap)::iterator> queue;
		
		ret.reserve(64);
		// wish I could `reserve()` queue...
		
		std::size_t addedCount = 0;
		
		for(auto it = blockMap.begin(); it != blockMap.end(); it++) {
			SPAssert(queue.empty());
			
			if(!it->second) continue;
			queue.emplace_back(it);
			
			std::vector<client::CellPos> outBlocks;
			
			while(!queue.empty()) {
				auto blockitem = queue.front();
				queue.pop_front();
				
				if(!blockitem->second) continue;
				
				auto pos = blockitem->first;
				outBlocks.emplace_back(pos);
				blockitem->second = false;
				
				decltype(blockMap)::iterator nextIt;
				
				nextIt = blockMap.find(CellPos(pos.x - 1, pos.y, pos.z));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
				nextIt = blockMap.find(CellPos(pos.x + 1, pos.y, pos.z));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
				nextIt = blockMap.find(CellPos(pos.x, pos.y - 1, pos.z));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
				nextIt = blockMap.find(CellPos(pos.x, pos.y + 1, pos.z));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
				nextIt = blockMap.find(CellPos(pos.x, pos.y, pos.z - 1));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
				nextIt = blockMap.find(CellPos(pos.x, pos.y, pos.z + 1));
				if(nextIt != blockMap.end() && nextIt->second) {
					queue.emplace_back(nextIt);
				}
			}
			
			SPAssert(!outBlocks.empty());
			addedCount += outBlocks.size();
			ret.emplace_back(std::move(outBlocks));
		}
		
		SPAssert(addedCount == blocks.size());
		
		return std::move(ret);
	}

	
	void World::FlushMapEdits() {
		// ReceivedMapEdits, which is called by client, uses
		// this function, so don't assert IsLocalHostServer().
		using namespace client;
		
		if (mapEdits.empty()) return;
		
		// add blocks
		std::vector<CellPos> removedBlocks;
		std::unordered_set<CellPos, CellPosHash> removedBlocksSet;
		for (const auto& pair: mapEdits) {
			const auto& edit = pair.second;
			const auto& p = edit.position;
			if (edit.color) {
				if (gameMap->IsSolid(p.x, p.y, p.z)) {
					gameMap->Set(p.x, p.y, p.z, true, *edit.color);
					for (auto *listen: listeners)
						listen->BlockUpdated(IntVector3(p.x, p.y, p.z),
											 edit.createType);
				} else {
					gameMapWrapper->AddBlock(p.x, p.y, p.z,
											 *edit.color);
					for (auto *listen: listeners)
						listen->BlockCreated(IntVector3(p.x, p.y, p.z),
											 edit.createType);
				}
				
			} else {
				removedBlocks.emplace_back(p.x, p.y, p.z);
				removedBlocksSet.emplace(p.x, p.y, p.z);
				
				for (auto *listen: listeners)
					listen->BlockDestroyed(IntVector3(p.x, p.y, p.z),
										   edit.destroyType);
			}
		}
		
		// remove blocks
		auto fallen = gameMapWrapper->RemoveBlocks(removedBlocks);
		if (fallen.size() > 0) {
			auto clusters = ClusterizeBlocks(fallen);
			std::vector<IntVector3> blocks;
			
			for (const auto& cluster: clusters) {
				blocks.clear();
				for (const auto& p: cluster) {
					if (removedBlocksSet.find(p) != removedBlocksSet.end()) {
						blocks.emplace_back(p.x, p.y, p.z);
					}
				}
				if (!blocks.empty()) {
					for (auto *l: listeners)
						l->BlocksFalling(blocks);
				}
			}
			
			for (const auto& cluster: clusters) {
				for (const auto& p: cluster) {
					gameMap->Set(p.x, p.y, p.z, false, 0);
				}
			}
			
		} else {
			SPAssert(removedBlocks.empty());
		}
		
		std::vector<MapEdit> mapEditList;
		for (const auto& pair: mapEdits) {
			mapEditList.emplace_back(std::move(pair.second));
		}
		mapEdits.clear();
		
		for (auto *l: listeners)
			l->FlushMapEdits(mapEditList);
	}
	
	void World::ReceivedMapEdits(const std::vector<MapEdit> &edits) {
		for (const auto& edit: edits) {
			SPAssert(IsValidMapCoord(edit.position));
			mapEdits[edit.position] = edit;
		}
		FlushMapEdits();
	}
} }

