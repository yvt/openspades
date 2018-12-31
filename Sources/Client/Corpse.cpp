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

#include "Corpse.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include "GameMap.h"
#include "IModel.h"
#include "IRenderer.h"
#include "Player.h"
#include "World.h"

using namespace std;

DEFINE_SPADES_SETTING(r_corpseLineCollision, "1");

namespace spades {
	namespace client {
		Corpse::Corpse(IRenderer *renderer, GameMap *map, Player *p)
		    : renderer(renderer), map(map) {
			SPADES_MARK_FUNCTION();

			playerId = p->GetId();

			IntVector3 col = p->GetWorld()->GetTeam(p->GetTeamId()).color;
			color = MakeVector3(col.x / 255.f, col.y / 255.f, col.z / 255.f);

			bool crouch = p->GetInput().crouch;
			Vector3 front = p->GetFront();

			float yaw = atan2(front.y, front.x) + static_cast<float>(M_PI) * .5f;
			// float pitch = -atan2(front.z, sqrt(front.x * front.x + front.y * front.y));

			// lower axis
			Matrix4 lower = Matrix4::Translate(p->GetOrigin());
			lower = lower * Matrix4::Rotate(MakeVector3(0, 0, 1), yaw);

			Matrix4 torso;

			if (crouch) {
				lower = lower * Matrix4::Translate(0, 0, -0.4f);
				torso = lower * Matrix4::Translate(0, 0, -0.3f);

				SetNode(Torso1, torso * MakeVector3(0.4f, -.15f, 0.1f));
				SetNode(Torso2, torso * MakeVector3(-0.4f, -.15f, 0.1f));
				SetNode(Torso3, torso * MakeVector3(-0.4f, .8f, 0.7f));
				SetNode(Torso4, torso * MakeVector3(0.4f, .8f, 0.7f));

				SetNode(Leg1, lower * MakeVector3(-0.4f, .1f, 1.f));
				SetNode(Leg2, lower * MakeVector3(0.4f, .1f, 1.f));

				SetNode(Arm1, torso * MakeVector3(0.2f, -.4f, .2f));
				SetNode(Arm2, torso * MakeVector3(-0.2f, -.4f, .2f));

			} else {
				torso = lower * Matrix4::Translate(0, 0, -1.1f);

				SetNode(Torso1, torso * MakeVector3(0.4f, 0.f, 0.1f));
				SetNode(Torso2, torso * MakeVector3(-0.4f, 0.f, 0.1f));
				SetNode(Torso3, torso * MakeVector3(-0.4f, .0f, 1.f));
				SetNode(Torso4, torso * MakeVector3(0.4f, .0f, 1.f));

				SetNode(Leg1, lower * MakeVector3(-0.4f, .0f, 1.f));
				SetNode(Leg2, lower * MakeVector3(0.4f, .0f, 1.f));

				SetNode(Arm1, torso * MakeVector3(0.2f, -.4f, .2f));
				SetNode(Arm2, torso * MakeVector3(-0.2f, -.4f, .2f));
			}

			SetNode(Head,
			        (nodes[Torso1].pos + nodes[Torso2].pos) * .5f + MakeVector3(0, 0, -0.6f));
		}

		void Corpse::SetNode(NodeType n, spades::Vector3 v) {
			auto velNoise = [&] { return (SampleRandomFloat() - SampleRandomFloat()) * 2.0f; };

			SPAssert(n >= 0);
			SPAssert(n < NodeCount);

			nodes[n].pos = v;
			nodes[n].vel = MakeVector3(velNoise(), velNoise(), 0.f);
			nodes[n].lastPos = v;
			nodes[n].lastForce = MakeVector3(0, 0, 0);
		}
		void Corpse::SetNode(NodeType n, spades::Vector4 v) {
			SetNode(n, v.GetXYZ());
		}

		Corpse::~Corpse() {}

		void Corpse::Spring(NodeType n1, NodeType n2, float distance, float dt) {
			SPADES_MARK_FUNCTION_DEBUG();
			SPAssert(n1 >= 0);
			SPAssert(n1 < NodeCount);
			SPAssert(n2 >= 0);
			SPAssert(n2 < NodeCount);
			Node &a = nodes[n1];
			Node &b = nodes[n2];
			Vector3 diff = b.pos - a.pos;
			float dist = diff.GetLength();
			Vector3 force = diff.Normalize() * (distance - dist);
			force *= dt * 50.f;

			b.vel += force;
			a.vel -= force;

			b.pos += force / (dt * 50.f) * 0.5f;
			a.pos -= force / (dt * 50.f) * 0.5f;

			Vector3 velMid = (a.vel + b.vel) * .5f;
			float dump = 1.f - powf(.1f, dt);
			a.vel += (velMid - a.vel) * dump;
			b.vel += (velMid - b.vel) * dump;
		}
		void Corpse::Spring(NodeType n1a, NodeType n1b, NodeType n2, float distance, float dt) {
			SPADES_MARK_FUNCTION_DEBUG();
			SPAssert(n1a >= 0);
			SPAssert(n1a < NodeCount);
			SPAssert(n1b >= 0);
			SPAssert(n1b < NodeCount);
			SPAssert(n2 >= 0);
			SPAssert(n2 < NodeCount);
			Node &x = nodes[n1a];
			Node &y = nodes[n1b];
			Node &b = nodes[n2];
			Vector3 diff = b.pos - (x.pos + y.pos) * .5f;
			float dist = diff.GetLength();
			Vector3 force = diff.Normalize() * (distance - dist);
			force *= dt * 50.f;

			b.vel += force;
			force *= .5f;
			x.vel -= force;
			y.vel -= force;

			Vector3 velMid = (x.vel + y.vel) * .25f + b.vel * .5f;
			float dump = 1.f - powf(.05f, dt);
			x.vel += (velMid - x.vel) * dump;
			y.vel += (velMid - y.vel) * dump;
			b.vel += (velMid - b.vel) * dump;
		}

		static float MyACos(float v) {
			SPAssert(!isnan(v));
			if (v >= 1.f)
				return 0.f;
			if (v <= -1.f)
				return static_cast<float>(M_PI);
			float vv = acosf(v);
			if (isnan(vv)) {
				vv = acosf(v * .9999f);
			}
			SPAssert(!isnan(vv));
			return vv;
		}

		void Corpse::AngleSpring(NodeType base, NodeType n1id, NodeType n2id, float minDot,
		                         float maxDot, float dt) {
			Node &nBase = nodes[base];
			Node &n1 = nodes[n1id];
			Node &n2 = nodes[n2id];
			Vector3 d1 = n1.pos - nBase.pos;
			Vector3 d2 = n2.pos - nBase.pos;
			float ln1 = d1.GetLength();
			float ln2 = d2.GetLength();
			float dot = Vector3::Dot(d1, d2) / (ln1 * ln2 + 0.0000001f);

			if (dot >= minDot && dot <= maxDot)
				return;

			Vector3 diff = n2.pos - n1.pos;
			float strength = 0.f;

			Vector3 a1 = Vector3::Cross(d1, diff);
			a1 = Vector3::Cross(d1, a1).Normalize();

			Vector3 a2 = Vector3::Cross(d2, diff);
			a2 = Vector3::Cross(d2, a2).Normalize();

			// a1=-a1; a2=-a2;
			// a1 = -a1;
			a2 = -a2;

			if (dot > maxDot) {
				strength = MyACos(dot) - MyACos(maxDot);
			} else if (dot < minDot) {
				strength = MyACos(dot) - MyACos(minDot);
			}

			SPAssert(!isnan(strength));

			strength *= 20.f;
			strength *= dt;

			a1 *= strength;
			a2 *= strength;

			a2 *= 0.f;

			n2.vel += a1;
			n1.vel += a2;
			nBase.vel -= a1 + a2;

			/*
			d1 += a1 * 0.01;
			d2 += a2 * 0.01;
			float nd = Vector3::Dot(d1, d2) / (d1.GetLength() * d2.GetLength());

			if(dot > maxDot){
			    if(nd < dot)
			        printf("GOOD %f -> %f\n", dot, nd);
			    else
			        printf("BAD %f -> %f\n", dot, nd);
			}else{
			    if(nd > dot)
			        printf("GOOD %f -> %f\n", dot, nd);
			    else
			        printf("BAD %f -> %f\n", dot, nd);
			}*/
		}
		void Corpse::AngleSpring(NodeType n1id, NodeType n2id, Vector3 dir, float minDot,
		                         float maxDot, float dt) {
			Node &n1 = nodes[n1id];
			Node &n2 = nodes[n2id];
			Vector3 diff = n2.pos - n1.pos;
			float ln1 = diff.GetLength();
			float dot = Vector3::Dot(diff, dir) / (ln1 + 0.000000001f);

			if (dot >= minDot && dot <= maxDot)
				return;

			float strength = 0.f;

			Vector3 a1 = Vector3::Cross(dir, diff);
			a1 = Vector3::Cross(diff, a1).Normalize();

			Vector3 a2 = -a1;
			// a1=-a1; a2=-a2;
			// a1 = -a1;

			if (dot > maxDot) {
				strength = MyACos(dot) - MyACos(maxDot);
			} else if (dot < minDot) {
				strength = MyACos(dot) - MyACos(minDot);
			}

			SPAssert(!isnan(strength));

			strength *= 100.f;
			strength *= dt;

			a1 *= strength;
			a2 *= strength;

			n2.vel += a1;
			n1.vel += a2;
			// nBase.vel -= a1 + a2;

			/*
			 d1 += a1 * 0.01;
			 d2 += a2 * 0.01;
			 float nd = Vector3::Dot(d1, d2) / (d1.GetLength() * d2.GetLength());

			 if(dot > maxDot){
			 if(nd < dot)
			 printf("GOOD %f -> %f\n", dot, nd);
			 else
			 printf("BAD %f -> %f\n", dot, nd);
			 }else{
			 if(nd > dot)
			 printf("GOOD %f -> %f\n", dot, nd);
			 else
			 printf("BAD %f -> %f\n", dot, nd);
			 }*/
		}

		static float fractf(float v) { return v - floorf(v); }

		static void CheckEscape(GameMap *map, IntVector3 hitBlock, IntVector3 a, IntVector3 b,
		                        IntVector3 dir, float &bestDist, IntVector3 &bestDir) {
			hitBlock += dir;
			IntVector3 aa = a + dir;
			IntVector3 bb = b + dir;
			if (map->IsSolidWrapped(hitBlock.x, hitBlock.y, hitBlock.z))
				return;
			if (map->IsSolidWrapped(aa.x, aa.y, aa.z))
				return;
			if (map->IsSolidWrapped(bb.x, bb.y, bb.z))
				return;
			float dist;
			if (dir.x == 1) {
				dist = 1.f - fractf(a.x);
				dist += 1.f - fractf(b.x);
			} else if (dir.x == -1) {
				dist = fractf(a.x);
				dist += fractf(b.x);
			} else if (dir.y == 1) {
				dist = 1.f - fractf(a.y);
				dist += 1.f - fractf(b.y);
			} else if (dir.y == -1) {
				dist = fractf(a.y);
				dist += fractf(b.y);
			} else if (dir.z == 1) {
				dist = 1.f - fractf(a.z);
				dist += 1.f - fractf(b.z);
			} else if (dir.z == -1) {
				dist = fractf(a.z);
				dist += fractf(b.z);
			} else {
				SPAssert(false);
				return;
			}

			if (dist < bestDist) {
				bestDist = dist;
				bestDir = dir;
			}
		}

		void Corpse::LineCollision(NodeType a, NodeType b, float dt) {
			if (!r_corpseLineCollision)
				return;
			Node &n1 = nodes[a];
			Node &n2 = nodes[b];

			IntVector3 hitBlock;

			if (map->CastRay(n1.lastPos, n2.lastPos, 16.f, hitBlock)) {
				GameMap::RayCastResult res1 = map->CastRay2(n1.lastPos, n2.lastPos - n1.lastPos, 8);
				GameMap::RayCastResult res2 = map->CastRay2(n2.lastPos, n1.lastPos - n2.lastPos, 8);

				if (!res1.hit)
					return;
				if (!res2.hit)
					return;
				if (res1.startSolid || res2.startSolid) {
					return;
				}

				// really hit?
				if (Vector3::Dot(res1.hitPos - n1.lastPos, n2.lastPos - n1.lastPos) >
				    (n2.pos - n1.pos).GetPoweredLength()) {
					return;
				}
				if (Vector3::Dot(res1.hitPos - n1.lastPos, n2.lastPos - n1.lastPos) < 0.f) {
					return;
				}
				if (Vector3::Dot(res2.hitPos - n2.lastPos, n1.lastPos - n2.lastPos) >
				    (n2.pos - n1.pos).GetPoweredLength()) {
					return;
				}
				if (Vector3::Dot(res2.hitPos - n2.lastPos, n1.lastPos - n2.lastPos) < 0.f) {
					return;
				}

				Vector3 blockPos;
				blockPos.x = hitBlock.x + .5f;
				blockPos.y = hitBlock.y + .5f;
				blockPos.z = hitBlock.z + .5f;

				float inlen = (res1.hitPos - res2.hitPos).GetLength();

				IntVector3 ivec = {0, 0, 0};

				ivec.x += res1.normal.x;
				ivec.y += res1.normal.y;
				ivec.z += res1.normal.z;
				ivec.x += res2.normal.x;
				ivec.y += res2.normal.y;
				ivec.z += res2.normal.z;

				Vector3 dir = {0.f, 0.f, 0.f};
				if (ivec.x == 0 && ivec.y == 0 && ivec.z == 0) {
					// hanging. which direction to escape?
					float bestDist = 1000.f;
					IntVector3 bestDir;
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(1, 0, 0), bestDist, bestDir);
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(-1, 0, 0), bestDist, bestDir);
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(0, 1, 0), bestDist, bestDir);
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(0, -1, 0), bestDist, bestDir);
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(0, 0, 1), bestDist, bestDir);
					CheckEscape(map, hitBlock, n1.pos.Floor(), n2.pos.Floor(),
					            IntVector3::Make(0, 0, -1), bestDist, bestDir);
					if (bestDist > 10.f) {
						// failed to find appropriate direction.
						return;
					}
					ivec = bestDir;
					inlen = bestDist + .1f;
				}
				dir.x = ivec.x;
				dir.y = ivec.y;
				dir.z = ivec.z;

				Vector3 normDir = dir; // |D|

				n1.vel -= normDir * std::min(Vector3::Dot(normDir, n1.vel), 0.f);
				n2.vel -= normDir * std::min(Vector3::Dot(normDir, n2.vel), 0.f);

				dir *= dt * inlen * 5.f;

				n1.vel += dir;
				n2.vel += dir;

				// friction
				n1.vel -= (n1.vel - normDir * Vector3::Dot(normDir, n1.vel)) * .2f;
				n2.vel -= (n2.vel - normDir * Vector3::Dot(normDir, n2.vel)) * .2f;
			}
		}

		void Corpse::AngularMomentum(int eId, NodeType a, NodeType b) {
			Edge &e = edges[eId];
			e.velDiff = nodes[b].vel - nodes[a].vel;
			if (e.node1 != a || e.node2 != b) {
				e.lastVelDiff = e.velDiff;
				e.node1 = a;
				e.node2 = b;
				return;
			}

			Vector3 force = e.lastVelDiff - e.velDiff;
			force *= .5f;
			nodes[b].vel += force;
			nodes[a].vel -= force;

			e.lastVelDiff = e.velDiff;
		}

		void Corpse::ApplyConstraint(float dt) {
			SPADES_MARK_FUNCTION();

			AngularMomentum(0, Torso1, Torso2);
			AngularMomentum(1, Torso2, Torso3);
			AngularMomentum(2, Torso3, Torso4);
			AngularMomentum(3, Torso4, Torso1);
			AngularMomentum(4, Torso1, Arm1);
			AngularMomentum(5, Torso2, Arm2);
			AngularMomentum(6, Torso3, Leg1);
			AngularMomentum(7, Torso4, Leg2);

			Spring(Torso1, Torso2, 0.8f, dt);
			Spring(Torso3, Torso4, 0.8f, dt);

			Spring(Torso1, Torso4, 0.9f, dt);
			Spring(Torso2, Torso3, 0.9f, dt);

			Spring(Torso1, Torso3, 1.204f, dt);
			Spring(Torso2, Torso4, 1.204f, dt);

			Spring(Arm1, Torso1, 1.f, dt);
			Spring(Arm2, Torso2, 1.f, dt);
			Spring(Leg1, Torso3, 1.f, dt);
			Spring(Leg2, Torso4, 1.f, dt);

			AngleSpring(Torso1, Arm1, Torso3, -1.f, 0.6f, dt);
			AngleSpring(Torso2, Arm2, Torso4, -1.f, 0.6f, dt);

			AngleSpring(Torso3, Leg1, Torso2, -1.f, -0.2f, dt);
			AngleSpring(Torso4, Leg2, Torso1, -1.f, -0.2f, dt);

			Spring(Torso1, Torso2, Head, .6f, dt);
			/*
			AngleSpring(Torso1,
			            Torso2, Head,
			            0.5f, 1.f, dt);

			AngleSpring(Torso2,
			            Torso1, Head,
			            0.5f, 1.f, dt);
			 */

			LineCollision(Torso1, Torso2, dt);
			LineCollision(Torso2, Torso3, dt);
			LineCollision(Torso3, Torso4, dt);
			LineCollision(Torso4, Torso1, dt);
			LineCollision(Torso1, Torso3, dt);
			LineCollision(Torso2, Torso4, dt);
			LineCollision(Torso1, Arm1, dt);
			LineCollision(Torso2, Arm2, dt);
			LineCollision(Torso3, Leg1, dt);
			LineCollision(Torso4, Leg2, dt);
		}

		void Corpse::Update(float dt) {
			SPADES_MARK_FUNCTION();
			float damp = 1.f;
			float damp2 = 1.f;
			if (dt > 0.f) {
				damp = powf(.9f, dt);
				damp2 = powf(.371f, dt);
			}
			// dt *= 0.1f;

			for (int i = 0; i < NodeCount; i++) {
				Node &node = nodes[i];
				Vector3 oldPos = node.lastPos;
				node.pos += node.vel * dt;

				SPAssert(!isnan(node.pos.x));
				SPAssert(!isnan(node.pos.y));
				SPAssert(!isnan(node.pos.z));

				if (node.pos.z > 63.f) {
					node.vel.z -= dt * 6.f; // buoyancy
					node.vel *= damp;
				} else {
					node.vel.z += dt * 32.f; // gravity
					node.vel.z *= damp2;
				}

				// node.vel *= damp;

				if (!map->ClipBox(oldPos.x, oldPos.y, oldPos.z)) {

					if (map->ClipBox(node.pos.x, oldPos.y, oldPos.z)) {
						node.vel.x = -node.vel.x * .2f;
						if (fabsf(node.vel.x) < .3f)
							node.vel.x = 0.f;
						node.pos.x = oldPos.x;

						node.vel.y *= .5f;
						node.vel.z *= .5f;
					}

					if (map->ClipBox(node.pos.x, node.pos.y, oldPos.z)) {
						node.vel.y = -node.vel.y * .2f;
						if (fabsf(node.vel.y) < .3f)
							node.vel.y = 0.f;
						node.pos.y = oldPos.y;

						node.vel.x *= .5f;
						node.vel.z *= .5f;
					}

					if (map->ClipBox(node.pos.x, node.pos.y, node.pos.z)) {
						node.vel.z = -node.vel.z * .2f;
						if (fabsf(node.vel.z) < .3f)
							node.vel.z = 0.f;
						node.pos.z = oldPos.z;

						node.vel.x *= .5f;
						node.vel.y *= .5f;
					}

					if (map->ClipBox(node.pos.x, node.pos.y, node.pos.z)) {
						// TODO: getting out block
						// node.pos = oldPos;
						// node.vel *= .5f;
					}
				}

				/*
				if(map->ClipBox(node.pos.x,
				                node.pos.y,
				                node.pos.z)){
				    if(!map->ClipBox(node.pos.x,
				                    node.pos.y,
				                    oldPos.z)){
				        node.vel.z = -node.vel.z * .2f;
				        if(fabsf(node.vel.z) < .3f)
				            node.vel.z = 0.f;
				        node.pos.z = oldPos.z;
				    }
				    if(!map->ClipBox(node.pos.x,
				                     oldPos.y,
				                     node.pos.z)){
				        node.vel.y = -node.vel.y * .2f;
				        if(fabsf(node.vel.y) < .3f)
				            node.vel.y = 0.f;
				        node.pos.y = oldPos.y;
				    }
				    if(!map->ClipBox(oldPos.x,
				                     node.pos.y,
				                     node.pos.z)){
				        node.vel.x = -node.vel.x * .2f;
				        if(fabsf(node.vel.x) < .3f)
				            node.vel.x = 0.f;
				        node.pos.x = oldPos.x;
				    }
				    node.vel *= .8f;
				    //node.pos = oldPos;

				    if(node.vel.GetLength() < .02f){
				        node.vel *= 0.f;
				    }
				}*/

				node.lastPos = node.pos;
				node.lastForce = node.vel;
			}
			ApplyConstraint(dt);

			for (int i = 0; i < NodeCount; i++) {
				nodes[i].lastForce = nodes[i].vel - nodes[i].lastForce;
			}
		}

		void Corpse::AddToScene() {
			ModelRenderParam param;
			param.customColor = color;

			IModel *model;
			Matrix4 scaler = Matrix4::Scale(.1f);

			// draw torso
			Matrix4 torso;
			Vector3 tX, tY;
			{
				Vector3 tX1 = nodes[Torso1].pos - nodes[Torso2].pos;
				Vector3 tX2 = nodes[Torso4].pos - nodes[Torso3].pos;
				Vector3 tY1 = nodes[Torso1].pos + nodes[Torso2].pos;
				Vector3 tY2 = nodes[Torso4].pos + nodes[Torso3].pos;
				tX = ((tX1 + tX2) * .5f).Normalize();
				tY = ((tY2 - tY1) * .5f).Normalize();
				Vector3 tZ = Vector3::Cross(tX, tY).Normalize();
				tY = Vector3::Cross(tX, tZ).Normalize();
				Vector3 tOrigin = tY1 * .5f;
				torso = Matrix4::FromAxis(tX, -tZ, -tY, tOrigin);

				param.matrix = torso * scaler;

				model = renderer->RegisterModel("Models/Player/Torso.kv6");
				renderer->RenderModel(model, param);
			}
			// draw Head
			{
				Vector3 headBase = (torso * MakeVector3(0.0f, 0.f, 0.f)).GetXYZ();

				model = renderer->RegisterModel("Models/Player/Head.kv6");

				Vector3 aX, aY, aZ;
				Vector3 center = (nodes[Torso1].pos + nodes[Torso2].pos) * .5f;

				aZ = nodes[Head].pos - center;
				aZ = -torso.GetAxis(2);
				aZ = aZ.Normalize();
				aY = nodes[Torso2].pos - nodes[Torso1].pos;
				aY = Vector3::Cross(aY, aZ).Normalize();
				aX = Vector3::Cross(aY, aZ).Normalize();
				param.matrix = Matrix4::FromAxis(-aX, aY, -aZ, headBase) * scaler;

				renderer->RenderModel(model, param);
			}

			// draw Arms
			{
				Vector3 arm1Base = (torso * MakeVector3(0.4f, 0.f, 0.2f)).GetXYZ();
				Vector3 arm2Base = (torso * MakeVector3(-0.4f, 0.f, 0.2f)).GetXYZ();

				model = renderer->RegisterModel("Models/Player/Arm.kv6");

				Vector3 aX, aY, aZ;

				aZ = nodes[Arm1].pos - nodes[Torso1].pos;
				aZ = aZ.Normalize();
				aY = nodes[Torso2].pos - nodes[Torso1].pos;
				aY = Vector3::Cross(aY, aZ).Normalize();
				aX = Vector3::Cross(aY, aZ).Normalize();
				param.matrix = Matrix4::FromAxis(aX, aY, aZ, arm1Base) * scaler;

				renderer->RenderModel(model, param);

				aZ = nodes[Arm2].pos - nodes[Torso2].pos;
				aZ = aZ.Normalize();
				aY = nodes[Torso1].pos - nodes[Torso2].pos;
				aY = Vector3::Cross(aY, aZ).Normalize();
				aX = Vector3::Cross(aY, aZ).Normalize();
				param.matrix = Matrix4::FromAxis(aX, aY, aZ, arm2Base) * scaler;

				renderer->RenderModel(model, param);
			}

			// draw Leg
			{
				Vector3 leg1Base = (torso * MakeVector3(0.25f, 0.f, 0.9f)).GetXYZ();
				Vector3 leg2Base = (torso * MakeVector3(-0.25f, 0.f, 0.9f)).GetXYZ();

				model = renderer->RegisterModel("Models/Player/Leg.kv6");

				Vector3 aX, aY, aZ;

				aZ = nodes[Leg1].pos - nodes[Torso3].pos;
				aZ = aZ.Normalize();
				aY = nodes[Torso1].pos - nodes[Torso2].pos;
				aY = Vector3::Cross(aY, aZ).Normalize();
				aX = Vector3::Cross(aY, aZ).Normalize();
				param.matrix = Matrix4::FromAxis(aX, aY, aZ, leg1Base) * scaler;

				renderer->RenderModel(model, param);

				aZ = nodes[Leg2].pos - nodes[Torso4].pos;
				aZ = aZ.Normalize();
				aY = nodes[Torso1].pos - nodes[Torso2].pos;
				aY = Vector3::Cross(aY, aZ).Normalize();
				aX = Vector3::Cross(aY, aZ).Normalize();
				param.matrix = Matrix4::FromAxis(aX, aY, aZ, leg2Base) * scaler;

				renderer->RenderModel(model, param);
			}
		}

		Vector3 Corpse::GetCenter() {
			Vector3 v = {0, 0, 0};
			for (int i = 0; i < NodeCount; i++)
				v += nodes[i].pos;
			v *= 1.f / (float)NodeCount;
			return v;
		}

		bool Corpse::IsVisibleFrom(spades::Vector3 eye) {
			// distance culled?
			if ((eye - GetCenter()).GetLength() > 150.f)
				return false;

			for (int i = 0; i < NodeCount; i++) {
				IntVector3 outBlk;
				if (map->CastRay(eye, nodes[i].pos, 256.f, outBlk))
					return true;
			}
			return false;
		}

		void Corpse::AddImpulse(spades::Vector3 v) {
			for (int i = 0; i < NodeCount; i++)
				nodes[i].vel += v;
		}
	}
}
