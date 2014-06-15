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

#include "Entity.h"
#include "World.h"
#include <Core/Strings.h>

namespace spades { namespace game {
	
	Entity::Entity(World& world, EntityType type):
	world(world),
	type(type),
	health(100) {
		trajectory.type = TrajectoryType::Constant;
		trajectory.origin = Vector3(0, 0, 0);
		trajectory.angle = Quaternion(0.f, 0.f, 0.f, 1.f);
	}
	
	Entity::~Entity() {
		
	}
	
	void Entity::AddListener(EntityListener *l) {
		listeners.insert(l);
	}
	void Entity::RemoveListener(EntityListener *l) {
		listeners.erase(l);
	}
	
	void Entity::SetId(stmp::optional<uint32_t> entityId) {
		this->entityId = entityId;
		if (!entityId) {
			for (auto *listener: listeners)
				listener->Unlinked(*this);
		}
	}
	
	void Entity::Advance(Duration dt) {
		EvaluateTrajectory(dt);
	}
	
	std::string Entity::GetName() {
		std::string typeName = GetEntityTypeName(type);
		return Format("#{0} {1}",
					  entityId ? std::to_string(*entityId) : "(null)",
					  typeName);
	}
	
	void Entity::EvaluateTrajectory(Duration dt) {
		switch (trajectory.type) {
			case TrajectoryType::Constant:
				// nothing to do
				break;
			case TrajectoryType::Linear:
				trajectory.origin += trajectory.velocity * dt;
				trajectory.angle *= Quaternion::MakeRotation
				(trajectory.angularVelocity * dt);
				break;
			case TrajectoryType::Gravity:
				trajectory.origin += trajectory.velocity * dt;
				trajectory.origin += world.GetGravity() * (dt * dt * 0.5f);
				trajectory.velocity += world.GetGravity() * dt;
				trajectory.angle *= Quaternion::MakeRotation
				(trajectory.angularVelocity * dt);
				break;
			case TrajectoryType::Player:
				// non-player entity shouldn't have this trajectory type.
				SPAssert(false);
				break;
			case TrajectoryType::RigidBody:
				// we need a physics engine...
				SPNotImplemented();
				break;
			default:
				SPAssert(false);
		}
	}
	
	Matrix4 Entity::GetMatrix() const {
		Matrix4 m;
		switch (trajectory.type) {
			default:
				SPAssert(false);
			case TrajectoryType::Constant:
			case TrajectoryType::Gravity:
			case TrajectoryType::Linear:
			case TrajectoryType::RigidBody:
				m = trajectory.angle.ToRotationMatrix();
				break;
			case TrajectoryType::Player:
				auto ang = trajectory.eulerAngle;
				m = Matrix4::Rotate(Vector3(0.f, 0.f, 1.f), ang.z);
				m = Matrix4::Rotate(Vector3(1.f, 0.f, 0.f), ang.x) * m;
				m = Matrix4::Rotate(Vector3(0.f, 1.f, 0.f), ang.y) * m;
				break;
		}
		m *= Matrix4::Translate(trajectory.origin);
		return m;
	}
	
	void Entity::PerformAction(EntityEventType type, uint64_t param) {
		for (auto *listener: listeners)
			listener->ActionPerformed(*this, type, param);
		EventTriggered(type, param);
	}
	
	void Entity::EventTriggered(EntityEventType type, uint64_t param) {
		SPLog("Unsupported event {0} (param = {1}) for entity {2}",
			  GetEntityEventTypeName(type).c_str(), param, GetName().c_str());
	}
	
	void Entity::InflictDamage(const DamageInfo& info,
							   int amount) {
		SPAssert(info.toEntity == *entityId);
		
		for (auto *listener: listeners)
			listener->InflictDamage(*this, info);
		Damaged(info, amount);
	}
	
	void Entity::Damaged(const DamageInfo& info,
						 int amount) {
		health = std::max(0, health - amount);
	}
	
	bool Entity::IsLocallyControlled() {
		auto owner = GetOwnerPlayerId();
		auto local = GetWorld().GetLocalPlayerId();
		if (owner && local) {
			return *owner == *local;
		} else if (!owner && !local) {
			return true;
		} else {
			return false;
		}
	}
	
	stmp::optional<uint32_t> Entity::GetOwnerPlayerId() {
		return stmp::optional<uint32_t>();
	}
	
} }


