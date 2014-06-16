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

#include "PlayerEntity.h"
#include <Core/Strings.h>
#include "World.h"
#include <Core/Debug.h>

namespace spades { namespace game {
	PlayerEntity::PlayerEntity(World& world):
	Entity(world, EntityType::Player),
	tool(ToolSlot::Weapon1),
	blockColor(192, 192, 192) {
		SPADES_MARK_FUNCTION();
		
		GetTrajectory().type = TrajectoryType::Player;
	}
	PlayerEntity::~PlayerEntity() {
		
	}
	
	void PlayerEntity::AddListener(PlayerEntityListener *l) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(l);
		listeners.insert(l);
	}
	void PlayerEntity::RemoveListener(PlayerEntityListener *l) {
		listeners.erase(l);
	}
	
	float PlayerEntity::GetBoxSize() {
		return 0.45f;
	}
	
	float PlayerEntity::GetHeight(PlayerStance stance) {
		SPADES_MARK_FUNCTION();
		
		switch (stance) {
			case PlayerStance::Standing: return 2.72f;
			case PlayerStance::Crouch: return 1.72f;
			case PlayerStance::Prone: return 0.5f;
		}
		SPAssert(false);
		return 0.f;
	}
	
	float PlayerEntity::GetCurrentHeight() {
		return GetHeight(playerInput.stance);
	}
	
	std::string PlayerEntity::GetName() {
		SPADES_MARK_FUNCTION();
		
		// TODO: PlayerEntity::GetName
		return Format("{0} \"{1}\"", Entity::GetName(), "[player name here]");
	}
	
	bool PlayerEntity::IsLocallyControlled() {
		if (entityId && GetWorld().GetLocalPlayerId()) {
			return *entityId == *GetWorld().GetLocalPlayerId();
		} else {
			return false;
		}
	}
	
	void PlayerEntity::SetPlayerInput(const PlayerInput& input) {
		playerInput = input;
	}
	
	void PlayerEntity::UpdatePlayerInput(const PlayerInput& input) {
		SPADES_MARK_FUNCTION();
		
		SetPlayerInput(input);
		FixInput(playerInput);
		for (auto *listener: listeners) listener->PlayerInputUpdated(*this);
	}
	
	const PlayerWeapon& PlayerEntity::GetWeapon(int slot) {
		SPADES_MARK_FUNCTION();
		SPAssert(slot >= 0);
		SPAssert(slot < 3);
		return weapons[slot];
	}
	
	void PlayerEntity::SetWeapon(int slot, const PlayerWeapon& w) {
		SPADES_MARK_FUNCTION();
		SPAssert(slot >= 0);
		SPAssert(slot < 3);
		if (GetId()) {
			SPRaise("Cannot change weapon of player while linked to world");
		}
		weapons[slot] = w;
	}
	
	void PlayerEntity::SetBodySkin(const std::string &s) {
		SPADES_MARK_FUNCTION();
		if (GetId()) {
			SPRaise("Cannot change weapon of player while linked to world");
		}
		bodySkin = s;
	}
	
#pragma mark - Player Movement
	
	float PlayerEntity::GetJumpVelocity() {
		return GetWorld().GetParameters().playerJumpVelocity;
	}
	
	bool PlayerEntity::IsOnGroundOrWade() {
		return IsWading() || (!airborne);
	}
	
	bool PlayerEntity::IsWading() {
		return GetWorld().IsWaterAt(GetTrajectory().origin);
	}
	
	
	bool PlayerEntity::IsCurrentToolWeapon() {
		switch (tool) {
			case ToolSlot::Weapon1:
			case ToolSlot::Weapon2:
			case ToolSlot::Weapon3:
				return true;
			default:
				return false;
		}
	}
	
	bool PlayerEntity::FixInput(PlayerInput& inp) {
		SPADES_MARK_FUNCTION_DEBUG();
		
		bool fixed = false;
		if (inp.sprint && (inp.toolSecondary || inp.toolPrimary)) {
			// no tool can be used while sprinting
			fixed = true;
			inp.toolPrimary = inp.toolSecondary = false;
		}
		if ((inp.xmove != 0 || inp.ymove != 0) &&
			inp.stance == PlayerStance::Prone &&
			IsCurrentToolWeapon() &&
			inp.toolSecondary) {
			// cannot move while player is prone and aiming down the sight
			fixed = true;
			inp.xmove = inp.ymove = 0;
		}
		return fixed;
	}
	
	void PlayerEntity::EvaluateTrajectory(Duration dt) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(GetTrajectory().type == TrajectoryType::Player);
		
		if (FixInput(playerInput)) {
			for (auto *listener: listeners) listener->PlayerInputUpdated(*this);
		}
		
		if (GetFlags().fly) {
			FlyMove(dt);
		} else {
			WalkMove(dt);
		}
	}
	
	void PlayerEntity::WalkMove(Duration dt) {
		SPADES_MARK_FUNCTION();
		
		const auto& inp = playerInput;
		auto& traj = GetTrajectory();
		
		// on-ground movement
		Vector2 planeMove {0.f, 0.f};
		planeMove.x = static_cast<float>(inp.xmove) / 127.f;
		planeMove.y = static_cast<float>(inp.ymove) / 127.f;
		planeMove.x = std::max(planeMove.x, -1.f);
		planeMove.y = std::max(planeMove.y, -1.f);
		
		if (planeMove.GetPoweredLength() > 1.f)
			planeMove = planeMove.Normalize();
		
		if (!IsOnGroundOrWade()) {
			// airborne
			planeMove *= 0.1f;
		} else if (inp.stance == PlayerStance::Crouch) {
			planeMove *= 0.3f;
		} else if (inp.stance == PlayerStance::Prone) {
			planeMove *= 0.1f;
		}
		
		if (inp.toolSecondary && IsCurrentToolWeapon()) {
			planeMove *= 0.5f;
		}
		
		if (inp.sprint) {
			planeMove *= 1.3f;
		}
		
		if (IsWading()) {
			planeMove *= .33f;
		}
		
		auto m = GetMatrix();
		auto front = (m * Vector3(0.f, 1.f, 0.f)).GetXYZ();
		auto side  = (m * Vector3(1.f, 0.f, 0.f)).GetXYZ();
		
		front.z = 0.f; side.z = 0.f;
		if (front.GetPoweredLength() != 0.f &&
			side.GetPoweredLength() != 0.f) {
			front = front.Normalize();
			side = side.Normalize();
			
			traj.velocity += side * planeMove.x;
			traj.velocity += front * planeMove.y;
		}
		
		// frictions
		traj.velocity.z *= powf(.5f, dt);
		
		float groundFriction = .9f;
		if (IsWading()) {
			groundFriction = .5f;
		} else if (IsOnGroundOrWade()) {
			groundFriction = .1f;
		}
		groundFriction = powf(groundFriction, dt);
		traj.velocity.x *= groundFriction;
		traj.velocity.y *= groundFriction;
		
		BoxClipMove(dt);
		
		// footsteps
		if(traj.velocity.z >= 0.f && traj.velocity.z < .017f &&
		   inp.stance == PlayerStance::Standing &&
		   !(IsCurrentToolWeapon() && inp.toolSecondary) &&
		   IsOnGroundOrWade()){
			// count move distance
			float dx = traj.velocity.x * 3.6f;
			float dy = traj.velocity.y * 3.6f;
			float dist = sqrtf(dx*dx+dy*dy);
			moveDistance += dist;
			
			bool madeFootstep = false;
			while(moveDistance > 1.f){
				moveSteps++;
				moveDistance -= 1.f;
				
				if(!madeFootstep){
					for (auto *listener: listeners) listener->Footstep(*this);
					madeFootstep = true;
				}
			}
		}
		
	}
	
	void PlayerEntity::BoxClipMove(Duration dt) {
		SPADES_MARK_FUNCTION();
		
		auto& traj = GetTrajectory();
		auto& pos = traj.origin;
		auto& vel = traj.velocity;
		float nx = pos.x + vel.x * dt;
		float ny = pos.y + vel.y * dt;
		// TODO: BoxClipMove
		float size = GetBoxSize();
		float height = GetCurrentHeight();
		float climbableHeight = 1.5f;
		float slowdownFallVelocity = .25f;
		
		auto *map = GetWorld().GetGameMap();
		SPAssert(map);
		
		auto VerticalRayCast = [&](float x, float y, float z1, float z2) -> float {
			int xx = static_cast<int>(floorf(x));
			int yy = static_cast<int>(floorf(x));
			int zz = static_cast<int>(floorf(z1));
			int zz2 = static_cast<int>(floorf(z2));
			if (map->ClipBox(xx, yy, zz)) return z1;
			while (zz < zz2) {
				++zz;
				if (map->ClipBox(xx, yy, zz)) return zz;
			}
			return z2;
		};
		
		
		// X-movement
		float bx = nx + (vel.x > 0.f ? size : -size);
		float rz = std::min(VerticalRayCast(bx, pos.y - size, pos.z - height, pos.z + 1.f),
							VerticalRayCast(bx, pos.y + size, pos.z - height, pos.z + 1.f));
		if (rz > pos.z - climbableHeight) {
			pos.x = nx;
		} else {
			if (vel.x > 0.f) {
				pos.x = floorf(bx) - size;
			} else {
				pos.x = floorf(bx) + 1.f + size;
			}
			vel.x = 0.f;
		}
		
		
		// y-movement
		float by = ny + (vel.y > 0.f ? size : -size);
		rz = std::min(VerticalRayCast(pos.x + size, by, pos.z - height, pos.z + 1.f),
					  VerticalRayCast(pos.x - size, by, pos.z - height, pos.z + 1.f));
		if (rz > pos.z - climbableHeight) {
			pos.y = ny;
		} else {
			if (vel.y > 0.f) {
				pos.y = floorf(by) - size;
			} else {
				pos.y = floorf(by) + 1.f + size;
			}
			vel.y = 0.f;
		}
		
		// check climb
		rz = std::min({
			VerticalRayCast(pos.x + size, pos.y - size, pos.z - height, pos.z + 1.f),
			VerticalRayCast(pos.x - size, pos.y - size, pos.z - height, pos.z + 1.f),
			VerticalRayCast(pos.x + size, pos.y + size, pos.z - height, pos.z + 1.f),
			VerticalRayCast(pos.x - size, pos.y + size, pos.z - height, pos.z + 1.f)
		});
		float dz =  vel.z * dt;
		airborne = false;
		
		if (rz > pos.z + std::max(0.f, dz)) {
			// free fall
			vel += world.GetGravity() * dt;
			pos.z += dz;
			airborne = true;
		} else if (rz < pos.z - 0.01f) {
			// climb
			pos.z = std::max<float>(rz, pos.z - 1.f * dt);
		} else {
			// hit ground
			pos.z = rz;
			
			const auto& params = GetWorld().GetParameters();
			if (vel.z > params.fallDamageVelocity) {
				float p = vel.z - params.fallDamageVelocity;
				p /= (params.fatalFallDamageVelocity -
					  params.fallDamageVelocity);
				p = 1.f + p * 100.f;
				if (IsLocallyControlled()) {
					// FIXME: fell damage should be done by server
					//        for anti-cheating
					int amt = static_cast<int>(p);
					DamageInfo info;
					info.damageType = DamageType::Fall;
					info.firePosition = pos;
					info.hitPosition = pos;
					info.fromEntity = *GetId();
					info.toEntity = *GetId();
					InflictDamage(info, amt);
				}
				for (auto *listener: listeners)
					listener->Fell(*this, true);
			} else if (vel.z > slowdownFallVelocity) {
				for (auto *listener: listeners)
					listener->Fell(*this, false);
				vel *= .5f;
			}
			
			vel.z = 0.f;
		}
		
	}
	
	void PlayerEntity::FlyMove(Duration dt) {
		SPADES_MARK_FUNCTION();
		
		SPNotImplemented();
	}
	
	void PlayerEntity::EventTriggered(EntityEventType type, uint64_t param) {
		SPADES_MARK_FUNCTION();
		
		if (type == EntityEventType::Jump) {
			for (auto *listener: listeners) listener->Jumped(*this);
			if (IsOnGroundOrWade()) {
				GetTrajectory().velocity.z -= 0.36f;
			}
		} else {
			Entity::EventTriggered(type, param);
		}
	}
	
} }
