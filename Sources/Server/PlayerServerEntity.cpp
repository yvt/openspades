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

#include "PlayerServerEntity.h"

namespace spades { namespace server {
	
	PlayerServerEntity::PlayerServerEntity
	(game::PlayerEntity& entity, Server& server):
	ServerEntity(entity, server),
	entity(entity) {
		
	}
	
	PlayerServerEntity::~PlayerServerEntity() {
		
	}
	
	protocol::EntityUpdateItem
	PlayerServerEntity::Serialize() {
		SPADES_MARK_FUNCTION();
		
		protocol::EntityUpdateItem r =
		ServerEntity::Serialize();
		r.playerInput = entity.GetPlayerInput();
		r.tool = entity.GetTool();
		r.blockColor = entity.GetBlockColor();
		
		auto& cItem = *r.createItem;
		for (int i = 0; i < 3; i++) {
			const auto& w = entity.GetWeapon(i);
			cItem.weaponSkins[i] = w.skin;
			cItem.weaponParams[i] = w.param;
		}
		cItem.bodySkin = entity.GetBodySkin();
		
		return r;
	}
	
} }

