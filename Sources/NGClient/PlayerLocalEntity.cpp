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
#include <Core/Settings.h>

SPADES_SETTING(cg_fov, "");

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
	
	void PlayerLocalEntity::Fell(game::PlayerEntity &e,
								 bool hurt) {
		SPADES_MARK_FUNCTION();
		
		if (entity) return;
		SPAssert(&e == entity);
		
	}
	
	client::SceneDefinition PlayerLocalEntity::CreateSceneDefinition(client::IRenderer &renderer) {
		SPADES_MARK_FUNCTION();
		
		client::SceneDefinition def;
		
		float sw = renderer.ScreenWidth();
		float sh = renderer.ScreenHeight();
		
		float fov = (float)cg_fov * (M_PI / 180.f);
		
		def.fovY = fov;
		def.fovX = 2.f * atanf(tanf(fov * .5f) * sw / sh);
		
		def.viewOrigin = entity->GetTrajectory().origin;
		def.viewOrigin.z -= entity->GetCurrentHeight() - 0.2f;
		
		auto m = entity->GetMatrix();
		
		def.viewAxis[2] = (m * Vector4(0,1,0,0)).GetXYZ();
		def.viewAxis[1] = Vector3(0, 0, -1);
		def.viewAxis[0] = Vector3(1, 0, 0);
		/*
		def.viewAxis[2] += def.viewAxis[1] * sinf(arena.world->GetCurrentTime() * .3) * .02f;
		def.viewAxis[2] += def.viewAxis[0] * cosf(arena.world->GetCurrentTime() * .3) * .02f;*/
		
		def.viewAxis[0] = Vector3::Cross(def.viewAxis[2], def.viewAxis[1]).Normalize();
		def.viewAxis[1] = Vector3::Cross(def.viewAxis[0], def.viewAxis[2]).Normalize();
		def.viewAxis[2] = def.viewAxis[2].Normalize();
		def.zNear = .01f;
		def.skipWorld = false;
		def.depthOfFieldNearRange = 10.f;
		def.globalBlur = .0f;
		
		return def;
	}
	
} }
