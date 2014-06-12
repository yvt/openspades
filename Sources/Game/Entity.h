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
#include <set>

namespace spades { namespace game {
	
	class World;
	
	class PlayerEntity;
	class GrenadeEntity;
	class RocketEntity;
	class CommandPostEntity;
	class FlagEntity;
	class CheckpointEntity;
	class VehicleEntity;
	
	class EntityListener;
	class EntityVisitor;
	
	class Entity: public RefCountedObject {
		friend class PlayerEntity;
		friend class GrenadeEntity;
		friend class RocketEntity;
		friend class CommandPostEntity;
		friend class FlagEntity;
		friend class CheckpointEntity;
		friend class VehicleEntity;
		
		World& world;
		stmp::optional<uint32_t> entityId;
		
		std::set<EntityListener *> listeners;
		
		EntityType const type;
		EntityFlags flags;
		Trajectory trajectory;
		uint8_t health;
		
		virtual void EvaluateTrajectory(Duration);
		
		// private constructor pattern
		Entity(World& world, EntityType type);
	public:
		~Entity();
		
		World& GetWorld() const { return world; }
		
		void AddListener(EntityListener *l);
		void RemoveListener(EntityListener *l);
		
		stmp::optional<uint32_t> GetId() const { return entityId; }
		void SetId(stmp::optional<uint32_t> entityId);
		
		EntityType GetType() const { return type; }
		
		EntityFlags& GetFlags() { return flags; }
		
		Trajectory& GetTrajectory() { return trajectory; }
		Matrix4 GetMatrix() const;
		
		/** Returns a name for debugging. */
		virtual std::string GetName();
		
		void Advance(Duration);
		
		virtual bool IsLocallyControlled();
		
		void InflictDamage(const DamageInfo&, int amount);
		virtual void Damaged(const DamageInfo&, int amount);
		
		/** Causes this entity to do the specific action,
		 * and triggers the corresponding event (thus 
		 * EventTriggered will be called). */
		void PerformAction(EntityEventType,
						   uint64_t param);
		
		/** Called when a certain event is triggered.
		 * Calling this function solely doesn't cause any states
		 * transmitted over network. */
		virtual void EventTriggered(EntityEventType,
									uint64_t param);
		
		virtual void Accept(EntityVisitor&) = 0;
	};
	
	class EntityListener {
	public:
		virtual ~EntityListener() { }
		
		/** Called when entity performs an action, and
		 * it should be notified to the remote peer. */
		virtual void ActionPerformed(Entity&,
									 EntityEventType,
									 uint64_t param) { }
		
		virtual void InflictDamage(Entity&, const DamageInfo&) { }
		
		virtual void Damaged(Entity&, const DamageInfo&) { }
		
		virtual void Unlinked(Entity&) { }
	};
	
	class EntityVisitor {
	public:
		virtual ~EntityVisitor() { }
		virtual void Visit(PlayerEntity&) = 0;
		virtual void Visit(GrenadeEntity&) = 0;
		virtual void Visit(RocketEntity&) = 0;
		virtual void Visit(CommandPostEntity&) = 0;
		virtual void Visit(FlagEntity&) = 0;
		virtual void Visit(CheckpointEntity&) = 0;
		virtual void Visit(VehicleEntity&) = 0;
	};
	
} }

