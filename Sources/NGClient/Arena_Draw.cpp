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
#include <Core/Debug.h>
#include <Game/World.h>
#include <Core/Settings.h>
#include "LocalEntity.h"

SPADES_SETTING(cg_fov, "");

namespace spades { namespace ngclient {
	
	ArenaCamera& Arena::GetCamera() {
		SPADES_MARK_FUNCTION();
		
		// TODO: GetCamera
		return defaultCamera;
	}
	
	void Arena::SetupRenderer() {
		SPADES_MARK_FUNCTION();
		
		SPAssert(world->GetGameMap());
		// FIXME: delay the call to Initialize
		Initialize();
		
		renderer->SetFogDistance(160.f);
		renderer->SetFogColor(Vector3(.9f, .9f, .9f) * .3f);
		renderer->SetGameMap(world->GetGameMap());
		
		setupped = true;
	}
	
	void Arena::UnsetupRenderer() {
		SPADES_MARK_FUNCTION();
		
		if (!setupped) return;
		
		renderer->SetGameMap(nullptr);
		
		setupped = false;
	}
	
	client::SceneDefinition Arena::CreateSceneDefinition() {
		return GetCamera().CreateSceneDefinition(*renderer);
	}
	
	
	client::SceneDefinition Arena::DefaultCamera::CreateSceneDefinition
	(client::IRenderer& renderer) {
		client::SceneDefinition def;
		
		float sw = renderer.ScreenWidth();
		float sh = renderer.ScreenHeight();
		
		float fov = (float)cg_fov * (M_PI / 180.f);
		
		def.fovY = fov;
		def.fovX = 2.f * atanf(tanf(fov * .5f) * sw / sh);
		
		auto *map = arena.world->GetGameMap();
		SPAssert(map);
		def.viewOrigin = Vector3(map->Width() / 2,
								 map->Height() / 2 - 30,
								 -2);
		def.viewAxis[2] = Vector3(0, .4, 1);
		def.viewAxis[1] = Vector3(0, 1, 0);
		def.viewAxis[0] = Vector3(1, 0, 0);
		
		def.viewAxis[2] += def.viewAxis[1] * sinf(arena.world->GetCurrentTime() * .3) * .02f;
		def.viewAxis[2] += def.viewAxis[0] * cosf(arena.world->GetCurrentTime() * .3) * .02f;
		
		def.viewAxis[0] = Vector3::Cross(def.viewAxis[2], def.viewAxis[1]).Normalize();
		def.viewAxis[1] = Vector3::Cross(def.viewAxis[0], def.viewAxis[2]).Normalize();
		def.viewAxis[2] = def.viewAxis[2].Normalize();
		def.zNear = .01f;
		def.skipWorld = false;
		def.depthOfFieldNearRange = 50.f;
		def.globalBlur = .3f;
		
		return def;
	}
	
	
	
	void Arena::Render() {
		SPADES_MARK_FUNCTION();
		
		auto def = CreateSceneDefinition();
		
		renderer->StartScene(def);
		
		for (const auto& ent: localEntities) {
			ent->AddToScene();
		}
		
		renderer->EndScene();
		
	}
} }

