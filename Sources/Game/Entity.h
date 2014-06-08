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

#include <Core/RefCountedObject.h>
#include "Constants.h"
#include <Core/TMPUtils.h>

namespace spades { namespace game {
	
	class World;
	
	class Entity: public RefCountedObject {
		World& world;
		stmp::optional<uint32_t> entityId;
		
		EntityType const type;
		EntityFlags flags;
		Trajectory trajectory;
		uint8_t health;
		
		virtual void EvaluateTrajectory(Duration);
		
	public:
		Entity(World& world, EntityType type);
		~Entity();
		
		World& GetWorld() const { return world; }
		
		stmp::optional<uint32_t> GetId() const { return entityId; }
		void SetId(uint32_t entityId);
		
		EntityType GetType() const { return type; }
		
		EntityFlags& GetFlags() { return flags; }
		
		Trajectory& GetTrajectory() { return trajectory; }
		
		void Advance(Duration);
		
	};
	
} }

