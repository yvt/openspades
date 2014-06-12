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

#include "PlayerLocalEntity.h"
#include <Core/Debug.h>

namespace spades { namespace ngclient {
	
	PlayerLocalEntityFactory::PlayerLocalEntityFactory(Arena& arena):
	arena(arena) { }
	PlayerLocalEntityFactory::~PlayerLocalEntityFactory()
	{ }
	
	PlayerLocalEntity *PlayerLocalEntityFactory::Create(game::PlayerEntity& e) {
		return new PlayerLocalEntity(arena, e, *this);
	}
	
	PlayerLocalEntity::PlayerLocalEntity
	(Arena& arena,
	 game::PlayerEntity& entity,
	 PlayerLocalEntityFactory& factory):
	arena(arena),
	entity(&entity)
	{
		SPADES_MARK_FUNCTION();
		
		entity.AddListener(static_cast<game::EntityListener *>(this));
		entity.AddListener(static_cast<game::PlayerEntityListener *>(this));
	}
	
	PlayerLocalEntity::~PlayerLocalEntity() {
		SPADES_MARK_FUNCTION();
		
		if (entity) {
			entity->RemoveListener(static_cast<game::EntityListener *>(this));
			entity->RemoveListener(static_cast<game::PlayerEntityListener *>(this));
		}
	}
	
	bool PlayerLocalEntity::Update(game::Duration dt) {
		SPADES_MARK_FUNCTION();
		
		// TODO: Update
		return entity != nullptr;
	}
	
	void PlayerLocalEntity::AddToScene() {
		SPADES_MARK_FUNCTION();
		
		// TODO: AddToScene
	}
	
	void PlayerLocalEntity::Unlinked(game::Entity &e) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(&e == entity);
		
		// TODO: may be do something?
		//       leave corpse for ragdolls?
		//       death action must be done on "Death" event
		
		entity = nullptr;
	}
	
	void PlayerLocalEntity::Damaged(game::Entity &e,
									const game::DamageInfo &info) {
		SPADES_MARK_FUNCTION();
		
		if (entity) return;
		SPAssert(&e == entity);
	}
	
	
	void PlayerLocalEntity::Jumped(game::PlayerEntity &e) {
		SPADES_MARK_FUNCTION();
		
		if (entity) return;
		SPAssert(&e == entity);
		
	}
	
	void PlayerLocalEntity::Footstep(game::PlayerEntity &e) {
		SPADES_MARK_FUNCTION();
		
		if (entity) return;
		SPAssert(&e == entity);
	}
	
	void PlayerLocalEntity::Fell(game::PlayerEntity &ee,
								 bool hurt) {
		SPADES_MARK_FUNCTION();
		
		if (entity) return;
		SPAssert(&e == entity);
		
	}
	
	
	
} }
