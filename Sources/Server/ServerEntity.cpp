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

#include "ServerEntity.h"

namespace spades { namespace server {
	
	ServerEntity::ServerEntity(game::Entity& entity,
							   Server& server):
	entity(entity),
	server(server) {
		lastState.createItem = protocol::EntityCreateItem();
	}
	
	ServerEntity::~ServerEntity() {
		SPADES_MARK_FUNCTION();
		
	}
	
	void ServerEntity::Update(double) {
		SPADES_MARK_FUNCTION();
		
		// maybe nothing to do here...
	}
	
	void ServerEntity::SaveForDeltaEncoding() {
		SPADES_MARK_FUNCTION();
		
		lastState = Serialize();
		lastState.createItem.reset();
	}
	
	namespace {
		template<class T>
		stmp::optional<T> ComputeDelta
		(const stmp::optional<T>& old,
		 const stmp::optional<T>& cur) {
			if ((cur && !old) ||
				(cur && old && *cur != *old)) {
				return cur;
			}
			return stmp::optional<T>();
		}
	}
	
	stmp::optional<protocol::EntityUpdateItem>
	ServerEntity::DeltaSerialize() {
		SPADES_MARK_FUNCTION();
		
		auto current = Serialize();
		protocol::EntityUpdateItem ret;
		if (lastState.createItem)
			ret.createItem = current.createItem;
		ret.entityId = current.entityId;
		ret.flags = ComputeDelta(lastState.flags, current.flags);
		ret.trajectory = ComputeDelta(lastState.trajectory, current.trajectory);
		ret.health = ComputeDelta(lastState.health, current.health);
		ret.playerInput = ComputeDelta(lastState.playerInput, current.playerInput);
		ret.tool = ComputeDelta(lastState.tool, current.tool);
		ret.blockColor = ComputeDelta(lastState.blockColor, current.blockColor);
		
		if (!ret.createItem &&
			!ret.flags &&
			!ret.trajectory &&
			!ret.health &&
			!ret.playerInput &&
			!ret.tool &&
			!ret.blockColor) {
			return stmp::optional<protocol::EntityUpdateItem>();
		}
		
		return ret;
	}
	
	protocol::EntityUpdateItem
	ServerEntity::Serialize() {
		SPADES_MARK_FUNCTION();
		
		protocol::EntityUpdateItem r;
		
		protocol::EntityCreateItem c;
		c.type = entity.GetType();
		r.createItem = c;
		
		r.entityId = *entity.GetId();
		r.flags = entity.GetFlags();
		r.trajectory = entity.GetTrajectory();
		r.health = entity.GetHealth();
		return r;
	}
	
} }

