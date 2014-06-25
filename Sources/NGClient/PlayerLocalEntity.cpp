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
#include "Arena.h"
#include "Client.h"
#include <Client/IRenderer.h>

SPADES_SETTING(cg_fov, "");
SPADES_SETTING(cg_thirdperson, "0");

namespace spades { namespace ngclient {
	
	
	PlayerLocalEntityFactory::PlayerLocalEntityFactory(Arena& arena):
	arena(arena) {
		Handle<osobj::Loader> loader
		(new osobj::Loader("Models/Player/"), false);
		lower.Set(loader->LoadFrame("lower.osobj"), false);
		lowerRenderer.Set(new ModelTreeRenderer(arena.GetRenderer(), lower), false);
	}
	PlayerLocalEntityFactory::~PlayerLocalEntityFactory()
	{ }
	
	PlayerLocalEntity *PlayerLocalEntityFactory::Create(game::PlayerEntity& e) {
		return new PlayerLocalEntity(arena, e, *this);
	}
	
	class PlayerLocalEntity::WalkAnimator {
		static const int RightFoot = 0;
		static const int LeftFoot = 1;
		
		const float footSidePos = .2f;
		const float footDistanceLimit = .7f;
		
		std::array<bool, 2> footContactingGround { true, true };
		std::array<Vector3, 2> footPos;
		Vector3 origin { 0, 0, 0 };
		Vector3 velocity { 0, 0, 0 };
		float angle = 0;
		
		float floatTime = 0.f;
		
		Vector3 GetFront() {
			return Vector3(sinf(angle), cosf(angle), 0.f);
		}
		Vector3 GetRight() {
			return Vector3(cosf(angle), -sinf(angle), 0.f);
		}
		
		void Rotate(float& x, float& y, float axisX, float axisY,
					float angle) {
			float cx = x - axisX, cy = y - axisY;
			float c = cosf(angle), s = sinf(angle);
			x = axisX + cx * c - cy * s;
			y = axisY + cx * s + cy * c;
		}
		
		Vector3 GetNaturalPos(int foot) {
			auto p = origin;
			auto v = GetRight() * footSidePos;
			if (foot == RightFoot) {
				p += v;
			} else if (foot == LeftFoot) {
				p -= v;
			} else {
				SPAssert(0);
			}
			return p;
		}
		
		bool IsBothFootOnGround() {
			return footContactingGround[0] &&
			footContactingGround[1];
		}
		
	public:
		WalkAnimator() {
			
		}
		void SetPosition(Vector3 p, Vector3 v, float a) {
			if (footContactingGround[LeftFoot] &&
				!footContactingGround[RightFoot]) {
				Rotate(footPos[RightFoot].x,
					   footPos[RightFoot].y,
					   footPos[LeftFoot].x,
					   footPos[LeftFoot].y,
					   angle - a);
			} else if (footContactingGround[RightFoot] &&
					   !footContactingGround[LeftFoot]) {
				Rotate(footPos[LeftFoot].x,
					   footPos[LeftFoot].y,
					   footPos[RightFoot].x,
					   footPos[RightFoot].y,
					   angle - a);
			}
			
			origin = p;
			angle = a;
			velocity = v;
			origin.z = 0.f; // always zero
			velocity.z = 0.f;
		}
		void Reset() {
			footContactingGround[0] = true;
			footContactingGround[1] = true;
			footPos[0] = GetNaturalPos(0);
			footPos[1] = GetNaturalPos(1);
		}
		
		std::array<Vector3, 2> GetFootPos() {
			return footPos;
		}
		
		float GetDistanceFromNaturalState() {
			auto awayL = footPos[LeftFoot] - GetNaturalPos(LeftFoot);
			auto awayR = footPos[RightFoot] - GetNaturalPos(RightFoot);
			return awayL.GetLength() + awayR.GetLength();
		}
		
		float GetRotation() {
			auto awayL = footPos[LeftFoot] - GetNaturalPos(LeftFoot);
			auto awayR = footPos[RightFoot] - GetNaturalPos(RightFoot);
			auto front = GetFront();
			return Vector3::Dot(awayL - awayR, front);
		}
		
		void Update(float dt, int level = 0) {
			if (level > 10) {
				SPLog("WARNING: FootSimulation didn't converge.");
				SPLog("         origin = (%f, %f, %f)",
					  origin.x, origin.y, origin.z);
				SPLog("         velocity = (%f, %f, %f)",
					  velocity.x, velocity.y, velocity.z);
				SPLog("         footPos[LeftFoot] = (%f, %f, %f)",
					  footPos[LeftFoot].x, footPos[LeftFoot].y, footPos[LeftFoot].z);
				SPLog("         footPos[RightFoot] = (%f, %f, %f)",
					  footPos[RightFoot].x, footPos[RightFoot].y, footPos[RightFoot].z);
				SPLog("         footContactingGround[LeftFoot] = %s",
					  footContactingGround[LeftFoot] ? "true" : "false");
				SPLog("         footContactingGround[RightFoot] = %s",
					  footContactingGround[RightFoot] ? "true" : "false");
				return;
			}
			
			bool backpedal = Vector3::Dot(GetFront(), velocity) < -0.01f;
			float dir = backpedal ? -1.f : 1.f;
			
			// ensure legs are not detached
			{
				auto awayL = footPos[LeftFoot] - GetNaturalPos(LeftFoot);
				auto awayR = footPos[RightFoot] - GetNaturalPos(RightFoot);
				auto lenL = awayL.GetLength();
				auto lenR = awayR.GetLength();
				float limit = footDistanceLimit * 1.4f;
				if (lenL > limit) {
					awayL *= (lenL - limit) / lenL;
					footPos[LeftFoot] -= awayL;
				}
				if (lenR > limit) {
					awayR *= (lenR - limit) / lenR;
					footPos[RightFoot] -= awayR;
				}
			}
			
			// keep distance between legs
			{
				auto right = GetRight();
				auto awayL = footPos[LeftFoot] - GetNaturalPos(LeftFoot);
				auto dotL = Vector3::Dot(awayL, right);
				footPos[LeftFoot] -= right * (dotL * .5f);
				auto awayR = footPos[RightFoot] - GetNaturalPos(RightFoot);
				auto dotR = Vector3::Dot(awayR, right);
				footPos[RightFoot] -= right * (dotR * .5f);
			}
			
			if (IsBothFootOnGround()) {
				auto natR = GetNaturalPos(RightFoot);
				auto natL = GetNaturalPos(LeftFoot);
				bool awayR = (natR - footPos[RightFoot]).GetPoweredLength() > .08f;
				bool awayL = (natL - footPos[LeftFoot]).GetPoweredLength() > .08f;
				auto front = GetFront();
				
				int precedes = -1;
				
				floatTime = 0.f;
				
				if (precedes == -1 &&
					Vector3::Dot(footPos[LeftFoot] - natL,
								 front) * dir < -footDistanceLimit * .5f) {
					precedes = LeftFoot;
				}
				if (precedes == -1 &&
					Vector3::Dot(footPos[RightFoot] - natR,
								 front) * dir < -footDistanceLimit * .5f) {
					precedes = RightFoot;
				}
				if (precedes == -1) {
					float dt = Vector3::Dot(velocity, GetRight());
					if (fabsf(dt) > .5f) {
						dt = -dt;
					}
					if (dt > 0.f) {
						precedes = RightFoot;
					} else {
						precedes = LeftFoot;
					}
				}
				
				if (awayR && precedes == RightFoot) {
					footContactingGround[RightFoot] = false;
					return Update(dt, level + 1);
				} else if (awayL && precedes == LeftFoot) {
					footContactingGround[LeftFoot] = false;
					return Update(dt, level + 1);
				} else if (awayR) {
					footContactingGround[RightFoot] = false;
					return Update(dt, level + 1);
				} else if (awayL) {
					footContactingGround[LeftFoot] = false;
					return Update(dt, level + 1);
				}
			} else {
				float groundFootAwayness;
				int groundFoot = 0;
				int floatFoot = 0;
				if (footContactingGround[LeftFoot]) {
					groundFoot = LeftFoot;
					floatFoot = RightFoot;
				} else if (footContactingGround[RightFoot]) {
					groundFoot = RightFoot;
					floatFoot = LeftFoot;
				} else {
					SPAssert(false);
				}
				Vector3 groundAway = GetNaturalPos(groundFoot) -
									 footPos[groundFoot];
				groundFootAwayness = Vector3::Dot(groundAway, GetFront()) /
									 footDistanceLimit;
				groundFootAwayness = groundFootAwayness * .5f + .5f;
				groundFootAwayness = std::min(std::max(groundFootAwayness, 0.f), 1.f);
				
				if (backpedal)
					groundFootAwayness = 1.f - groundFootAwayness;
				
				bool isStopping = velocity.GetPoweredLength() < .1f;
				
				if(floatTime < .5f){
					Vector3 floatPos = GetNaturalPos(floatFoot);
					Vector3 offset = groundAway;
					offset -= offset * Vector3::Dot(offset, GetRight()) * .6f;
					
					float front = Vector3::Dot(offset, GetFront());
					offset = offset * 1.5f -
						offset * (0.5f * front * front / footDistanceLimit);
					
					floatPos += offset;
					
					float p = groundFootAwayness * (1.f - groundFootAwayness);
					floatPos.z -= p * 1.f;
					
					float speed = .5f;
					if (isStopping) {
						speed = .1f;
						floatPos.z *= .1f;
					}
					
					footPos[floatFoot].x += (floatPos.x - footPos[floatFoot].x) * speed;
					footPos[floatFoot].y += (floatPos.y - footPos[floatFoot].y) * speed;
					footPos[floatFoot].z += (floatPos.z - footPos[floatFoot].z) * .1f;
				} else {
					Vector3 floatPos = GetNaturalPos(floatFoot);
					footPos[floatFoot] += (floatPos - footPos[floatFoot]) * .04f;
					footPos[floatFoot].z = std::min
					(footPos[floatFoot].z + .1f * dt, 0.f);
					if ((footPos[floatFoot] - floatPos)
						.GetPoweredLength() < .001f) {
						footPos[floatFoot].z = 0.f;
						footContactingGround[floatFoot] = true;
						return Update(dt, level + 1);
					}
				}
				floatTime += dt;
				
				if (groundFootAwayness > .8f) {
					footPos[floatFoot].z = std::min
					(footPos[floatFoot].z + .2f * dt, 0.f);
					if (groundFootAwayness >= 1.f) {
						footPos[floatFoot].z = 0.f;
					}
					if (footPos[floatFoot].z >= 0.f) {
						footContactingGround[floatFoot] = true;
						return Update(dt, level + 1);
					}
				}
				
				// TODO: limit float time
			}
		}
	};
	
	PlayerLocalEntity::PlayerLocalEntity
	(Arena& arena,
	 game::PlayerEntity& entity,
	 PlayerLocalEntityFactory& factory):
	arena(arena),
	entity(&entity),
	walkAnim(new WalkAnimator()),
	factory(factory)
	{
		SPADES_MARK_FUNCTION();
		
		entity.AddListener(static_cast<game::EntityListener *>(this));
		entity.AddListener(static_cast<game::PlayerEntityListener *>(this));
		
		lowerPose.Set
		(new osobj::Pose(factory.lower), false);
		world.reset(new dWorld());
		lowerPhys.Set
		(new osobj::PhysicsObject(factory.lower,
								  world->id()),
		 false);
		world->setGravity(0, 0, 0);
		world->setERP(0.8);
		walkAnim->Reset();
	}
	
	PlayerLocalEntity::~PlayerLocalEntity() {
		SPADES_MARK_FUNCTION();
		
		lowerPhys.Set(nullptr);
		lowerPose.Set(nullptr);
		world.reset();
		
		if (entity) {
			entity->RemoveListener(static_cast<game::EntityListener *>(this));
			entity->RemoveListener(static_cast<game::PlayerEntityListener *>(this));
		}
	}
	
	bool PlayerLocalEntity::Update(game::Duration dt) {
		SPADES_MARK_FUNCTION();
		
		if (entity) {
			auto& traj = entity->GetTrajectory();
			walkAnim->SetPosition(traj.origin, traj.velocity, -traj.eulerAngle.z);
			walkAnim->Update(static_cast<float>(dt));
			
			for (int i = 0; i < 2; ++i) {
				
				auto Move = [&](dBody& b, const Vector3& v, bool force = false) {
					
					if (force) {
						b.setPosition(v.x, v.y, v.z);
						b.setAngularVel(0, 0, 0);
						b.setLinearVel(0, 0, 0);
					} else {
						auto f = v;
						auto *pos = b.getPosition();
						f.x -= pos[0]; f.y -= pos[1]; f.z -= pos[2];
						f *= 80.f;
						b.addForce(f.x, f.y, f.z);
					}
				};
				
				auto footPos = walkAnim->GetFootPos();
				
				float hop = walkAnim->GetDistanceFromNaturalState();
				hop = std::min(0.3f, hop * 0.05f);
				
				auto& frame = *factory.lower;
				auto *abd = frame.GetFrameById("abdomen");
				auto abdBody = lowerPhys->GetBody(abd);
				if (abdBody) {
					auto p = traj.origin;
					auto vel = traj.velocity; vel.z = 0;
					if (vel.GetLength() > 9.5f) vel = vel.Normalize() * 9.5f;
					p += vel * 0.05f;
					Move(*abdBody, Vector3(p.x, p.y, p.z - 1.3f + hop), true);
					
					auto wrot = walkAnim->GetRotation();
					auto q = Quaternion::MakeRotation(Vector3(0,0,-traj.eulerAngle.z + wrot * 0.1f));
					abdBody->setQuaternion(std::array<float, 4>{q.v.w, q.v.x, q.v.y, q.v.z}.data());
					
					abdBody->setKinematic();
					
				}
				
				auto *ftL = frame.GetFrameById("footLeft");
				auto ftLBody = lowerPhys->GetBody(ftL);
				if (ftLBody) {
					auto p = traj.origin;
					auto pp = footPos[0];
					Move(*ftLBody, Vector3(pp.x, pp.y, p.z + pp.z), false);
					//abdBody->setKinematic();
				}
				
				auto *ftR = frame.GetFrameById("footRight");
				auto ftRBody = lowerPhys->GetBody(ftR);
				if (ftRBody) {
					auto p = traj.origin;
					auto pp = footPos[1];
					Move(*ftRBody, Vector3(pp.x, pp.y, p.z + pp.z), false);
					//abdBody->setKinematic();
				}
				
				world->quickStep(0.1);
			}
			
			auto bods = lowerPhys->GetAllBodies();
			for (const auto& b: bods) {
				b->setDamping(.2, .2);/*
				b->setAngularVel(0, 0, 0);
				b->setLinearVel(0, 0, 0);*/
			}
		}
			
		// TODO: Update
		return entity != nullptr;
	}
	
	void PlayerLocalEntity::AddToScene() {
		SPADES_MARK_FUNCTION();
		
		if (entity) {
			lowerPhys->UpdatePose(*lowerPose);
			
			ModelTreeRenderParam param;
			param.customColor = Vector3(.2f, .6f, .2f);
			param.depthHack = false;
			
			factory.lowerRenderer->AddToScene(*lowerPose, param);
			
			
		}
			
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
		
		auto m = entity->GetMatrix();
		
		if (cg_thirdperson && arena.GetClient()->IsHostingServer()) {
			
			auto head = entity->GetTrajectory().origin;
			head.z -= 1.f; //entity->GetCurrentHeight() - 0.2f;
			
			def.viewOrigin = head + Vector3(0.f, 3.f, -1.f);
			
			def.viewAxis[2] = head - def.viewOrigin;
			def.viewAxis[1] = Vector3(0, 0, -1);
			
		} else {
		
			def.viewOrigin = entity->GetTrajectory().origin;
			def.viewOrigin.z -= entity->GetCurrentHeight() - 0.2f;
			
			def.viewAxis[2] = (m * Vector4(0,1,0,0)).GetXYZ();
			def.viewAxis[1] = Vector3(0, 0, -1);
			
		}
		
		def.viewAxis[0] = Vector3::Cross(def.viewAxis[2], def.viewAxis[1]).Normalize();
		def.viewAxis[1] = Vector3::Cross(def.viewAxis[0], def.viewAxis[2]).Normalize();
		def.viewAxis[2] = def.viewAxis[2].Normalize();
		def.zNear = .01f;
		def.skipWorld = false;
		def.depthOfFieldNearRange = 0.f;
		def.globalBlur = .0f;
		
		return def;
	}
	
} }
