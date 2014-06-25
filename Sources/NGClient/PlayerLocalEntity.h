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

#include "LocalEntity.h"
#include <Game/PlayerEntity.h>
#include "Arena.h"
#include "ModelTreeRenderer.h"
#include <Core/ModelPhysics.h>

namespace spades { namespace ngclient {
	
	class Arena;
	class PlayerLocalEntity;
	
	class PlayerLocalEntityFactory final {
		friend class PlayerLocalEntity;
		Arena& arena;
		Handle<osobj::Frame> lower;
		Handle<ModelTreeRenderer> lowerRenderer;
	public:
		PlayerLocalEntityFactory(Arena&);
		~PlayerLocalEntityFactory();
		
		PlayerLocalEntity *Create(game::PlayerEntity&);
	};
	
	/** Local entity for player entity.
	 * Note: not same as "local player" nor "local player entity".
	 *       "local player local entity" is a local entity for
	 *       the local player entity. */
	class PlayerLocalEntity final:
	public LocalEntity,
	game::EntityListener,
	game::PlayerEntityListener,
	public ArenaCamera
	{
		friend class PlayerLocalEntityFactory;
		
		Arena& arena;
		game::PlayerEntity *entity;
		PlayerLocalEntityFactory& factory;
		
		// used for inverse kinematics
		std::shared_ptr<dWorld> world;
		Handle<osobj::Pose> lowerPose;
		Handle<osobj::PhysicsObject> lowerPhys;
		
		class WalkAnimator;
		std::unique_ptr<WalkAnimator> walkAnim;
		
		void Damaged(game::Entity&, const game::DamageInfo&) override;
		void Unlinked(game::Entity&) override;
		void Jumped(game::PlayerEntity&) override;
		void Footstep(game::PlayerEntity&) override;
		void Fell(game::PlayerEntity&, bool hurt) override;
		PlayerLocalEntity(Arena& arena,
						  game::PlayerEntity&,
						  PlayerLocalEntityFactory&);
	public:
		virtual ~PlayerLocalEntity();
		virtual bool Update(game::Duration);
		virtual void AddToScene();
		
		client::SceneDefinition CreateSceneDefinition
		(client::IRenderer&) override;
		
	};
	
} }

