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

namespace spades { namespace game {
	
	Entity::Entity(World& world, EntityType type):
	world(world),
	type(type),
	health(100) {
		
	}
	
	Entity::~Entity() {
		
	}
	
	void Entity::SetId(uint32_t entityId) {
		this->entityId = entityId;
	}
	
	void Entity::Advance(Duration dt) {
		EvaluateTrajectory(dt);
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
} }


