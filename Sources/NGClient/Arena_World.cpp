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

#include "Arena.h"

#include <Game/Entity.h>
#include <Core/Debug.h>
#include "LocalEntity.h"
#include "PlayerLocalEntity.h"

namespace spades { namespace ngclient {
	void Arena::AddLocalEntity(LocalEntity *le) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(le);
		localEntities.emplace_front(le);
	}
	void Arena::AddLocalEntityForEntity
	(LocalEntity *le, game::Entity& e) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(le);
		localEntities.emplace_front(le);
		entityToLocalEntity.emplace(&e, localEntities.begin());
	}
	void Arena::EntityLinked(game::World& world,
							 game::Entity *e) {
		SPADES_MARK_FUNCTION();
		
		struct Factory: public game::EntityVisitor {
			Arena& arena;
			Factory(Arena& arena):
			arena(arena) { }
			void Visit(game::PlayerEntity& e) override {
				auto *le = arena.playerLocalEntityFactory->Create(e);
				arena.AddLocalEntityForEntity(le, e);
			}
			void Visit(game::GrenadeEntity&) override {
				SPNotImplemented();
			}
			void Visit(game::RocketEntity&) override {
				SPNotImplemented();
			}
			void Visit(game::CommandPostEntity&) override {
				SPNotImplemented();
			}
			void Visit(game::FlagEntity&) override {
				SPNotImplemented();
			}
			void Visit(game::CheckpointEntity&) override {
				SPNotImplemented();
			}
			void Visit(game::VehicleEntity&) override {
				SPNotImplemented();
			}
		};
		
		Factory f(*this);
		e->Accept(f);
	}
	void Arena::EntityUnlinked(game::World& world,
							   game::Entity *e) {
		SPADES_MARK_FUNCTION();
		
		// unlinked entities are usually handled by
		// local entites...
		entityToLocalEntity.erase(e);
	}
	void Arena::LoadEntities() {
		SPADES_MARK_FUNCTION();
		
		for (game::Entity *e: world->GetAllEntities()) {
			SPAssert(e);
			EntityLinked(*world, e);
		}
	}
	
	LocalEntity *Arena::GetLocalEntityForEntity(game::Entity*e) {
		if (!e) return nullptr;
		auto it = entityToLocalEntity.find(e);
		if (it == entityToLocalEntity.end())
			return nullptr;
		return (it->second)->get();
	}
	
	PlayerLocalEntity *Arena::GetLocalPlayerLocalEntity() {
		if (!world) return nullptr;
		auto *e = world->GetLocalPlayerEntity();
		if (!e) return nullptr;
		auto *le = GetLocalEntityForEntity(e);
		auto *ple = dynamic_cast<PlayerLocalEntity *>(le);
		SPAssert(ple);
		return ple;
	}
	
} }
