/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

#include "Client.h"

#include <Core/ConcurrentDispatch.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

#include "CTFGameMode.h"
#include "Corpse.h"
#include "GameMap.h"
#include "Grenade.h"
#include "IGameMode.h"
#include "Player.h"
#include "TCGameMode.h"
#include "Weapon.h"
#include "World.h"

#include "ClientPlayer.h"
#include "ILocalEntity.h"

#include "NetClient.h"

DEFINE_SPADES_SETTING(cg_fov, "68");
DEFINE_SPADES_SETTING(cg_thirdperson, "0");
DEFINE_SPADES_SETTING(cg_manualFocus, "0");
DEFINE_SPADES_SETTING(cg_depthOfFieldAmount, "1");
DEFINE_SPADES_SETTING(cg_shake, "1");

static float nextRandom() {
	return spades::real_dist(spades::client::mt_engine_client);
}

namespace spades {
	namespace client {

#pragma mark - Drawing

		bool Client::ShouldRenderInThirdPersonView() {
			if (world && world->GetLocalPlayer()) {
				if (!world->GetLocalPlayer()->IsAlive())
					return true;
			}
			if ((int)cg_thirdperson != 0 && world->GetNumPlayers() <= 1) {
				return true;
			}
			return false;
		}

		float Client::GetLocalFireVibration() {
			float localFireVibration = 0.f;
			localFireVibration = time - localFireVibrationTime;
			localFireVibration = 1.f - localFireVibration / 0.1f;
			if (localFireVibration < 0.f)
				localFireVibration = 0.f;
			return localFireVibration;
		}

		float Client::GetAimDownZoomScale() {
			if (world == nullptr || world->GetLocalPlayer() == nullptr ||
			    world->GetLocalPlayer()->IsToolWeapon() == false ||
			    world->GetLocalPlayer()->IsAlive() == false)
				return 1.f;
			float delta = .8f;
			switch (world->GetLocalPlayer()->GetWeapon()->GetWeaponType()) {
				case SMG_WEAPON: delta = .8f; break;
				case RIFLE_WEAPON: delta = 1.4f; break;
				case SHOTGUN_WEAPON: delta = .4f; break;
			}
			float aimDownState = GetAimDownState();
			return 1.f + (3.f - 2.f * powf(aimDownState, 1.5f)) * powf(aimDownState, 3.f) * delta;
		}

		SceneDefinition Client::CreateSceneDefinition() {
			SPADES_MARK_FUNCTION();

			int shakeLevel = cg_shake;

			SceneDefinition def;
			def.time = (unsigned int)(time * 1000.f);
			def.denyCameraBlur = true;
			def.zFar = 200.f;

			if (world) {
				IntVector3 fogColor = world->GetFogColor();
				renderer->SetFogColor(
				  MakeVector3(fogColor.x / 255.f, fogColor.y / 255.f, fogColor.z / 255.f));

				Player *player = world->GetLocalPlayer();

				def.blurVignette = .0f;

				if (IsFollowing()) {
					int limit = 100;
					// if current following player has left,
					// or removed,
					// choose next player.
					while (!world->GetPlayer(followingPlayerId) ||
					       world->GetPlayer(followingPlayerId)->GetFront().GetPoweredLength() <
					         .01f) {
						FollowNextPlayer(false);
						if ((limit--) <= 0) {
							break;
						}
					}
					player = world->GetPlayer(followingPlayerId);
				}
				if (player) {

					float roll = 0.f;
					float scale = 1.f;
					float vibPitch = 0.f;
					float vibYaw = 0.f;
					if (ShouldRenderInThirdPersonView() ||
					    (IsFollowing() && player != world->GetLocalPlayer())) {
						Vector3 center = player->GetEye();
						Vector3 playerFront = player->GetFront2D();
						Vector3 up = MakeVector3(0, 0, -1);

						if ((!player->IsAlive()) && lastMyCorpse &&
						    player == world->GetLocalPlayer()) {
							center = lastMyCorpse->GetCenter();
						}
						if (map->IsSolidWrapped((int)floorf(center.x), (int)floorf(center.y),
						                        (int)floorf(center.z))) {
							float z = center.z;
							while (z > center.z - 5.f) {
								if (!map->IsSolidWrapped((int)floorf(center.x),
								                         (int)floorf(center.y), (int)floorf(z))) {
									center.z = z;
									break;
								} else {
									z -= 1.f;
								}
							}
						}

						float distance = 5.f;
						if (player == world->GetLocalPlayer() &&
						    world->GetLocalPlayer()->GetTeamId() < 2 &&
						    !world->GetLocalPlayer()->IsAlive()) {
							// deathcam.
							float elapsedTime = time - lastAliveTime;
							distance -= 3.f * expf(-elapsedTime * 1.f);
						}

						Vector3 eye = center;
						// eye -= playerFront * 5.f;
						// eye += up * 2.0f;
						eye.x += cosf(followYaw) * cosf(followPitch) * distance;
						eye.y += sinf(followYaw) * cosf(followPitch) * distance;
						eye.z -= sinf(followPitch) * distance;

						if (false) {
							// settings for making limbo stuff
							eye = center;
							eye += playerFront * 3.f;
							eye += up * -.1f;
							eye += player->GetRight() * 2.f;
							scale *= .6f;
						}

						// try ray casting
						GameMap::RayCastResult result;
						result = map->CastRay2(center, (eye - center).Normalize(), 256);
						if (result.hit) {
							float dist = (result.hitPos - center).GetLength();
							float curDist = (eye - center).GetLength();
							dist -= 0.3f; // near clip plane
							if (curDist > dist) {
								float diff = curDist - dist;
								eye += (center - eye).Normalize() * diff;
							}
						}

						Vector3 front = center - eye;
						front = front.Normalize();

						if (firstPersonSpectate == false) {
							def.viewOrigin = eye;
							def.viewAxis[0] = -Vector3::Cross(up, front).Normalize();
							def.viewAxis[1] = -Vector3::Cross(front, def.viewAxis[0]).Normalize();
							def.viewAxis[2] = front;
						} else {
							def.viewOrigin = player->GetEye();
							def.viewAxis[0] = player->GetRight();
							def.viewAxis[1] = player->GetUp();
							def.viewAxis[2] = player->GetFront();
						}

						def.fovY = (float)cg_fov * static_cast<float>(M_PI) / 180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) * renderer->ScreenWidth() /
						                 renderer->ScreenHeight()) *
						           2.f;

						// update initial spectate pos
						// this is not used now, but if the local player is
						// is spectating, this is used when he/she's no
						// longer following
						followPos = def.viewOrigin;
						followVel = MakeVector3(0, 0, 0);

					} else if (player->GetTeamId() >= 2) {
						// spectator view (noclip view)
						Vector3 center = followPos;
						Vector3 front;
						Vector3 up = {0, 0, -1};

						front.x = -cosf(followYaw) * cosf(followPitch);
						front.y = -sinf(followYaw) * cosf(followPitch);
						front.z = sinf(followPitch);

						def.viewOrigin = center;
						def.viewAxis[0] = -Vector3::Cross(up, front).Normalize();
						def.viewAxis[1] = -Vector3::Cross(front, def.viewAxis[0]).Normalize();
						def.viewAxis[2] = front;

						def.fovY = (float)cg_fov * static_cast<float>(M_PI) / 180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) * renderer->ScreenWidth() /
						                 renderer->ScreenHeight()) *
						           2.f;

						// for 1st view, camera blur can be used
						def.denyCameraBlur = false;

					} else {
						Matrix4 eyeMatrix = clientPlayers[player->GetId()]->GetEyeMatrix();
						def.viewOrigin = eyeMatrix.GetOrigin();
						def.viewAxis[0] = -eyeMatrix.GetAxis(0);
						def.viewAxis[1] = -eyeMatrix.GetAxis(2);
						def.viewAxis[2] = eyeMatrix.GetAxis(1);

						if (shakeLevel >= 1) {
							float localFireVibration = GetLocalFireVibration();
							localFireVibration *= localFireVibration;

							if (player->GetTool() == Player::ToolSpade) {
								localFireVibration *= 0.4f;
							}

							roll += (nextRandom() - nextRandom()) * 0.03f * localFireVibration;
							scale += nextRandom() * 0.04f * localFireVibration;

							vibPitch += localFireVibration * (1.f - localFireVibration) * 0.01f;
							vibYaw += sinf(localFireVibration * (float)M_PI * 2.f) * 0.001f;

							def.radialBlur += localFireVibration * 0.05f;

							// sprint bob
							{
								float sp = SmoothStep(GetSprintState());
								vibYaw += sinf(player->GetWalkAnimationProgress() *
								               static_cast<float>(M_PI) * 2.f) *
								          0.01f * sp;
								roll -= sinf(player->GetWalkAnimationProgress() *
								             static_cast<float>(M_PI) * 2.f) *
								        0.005f * (sp);
								float p = cosf(player->GetWalkAnimationProgress() *
								               static_cast<float>(M_PI) * 2.f);
								p = p * p;
								p *= p;
								p *= p;
								vibPitch += p * 0.01f * sp;

								if (shakeLevel >= 2) {
									vibYaw += coherentNoiseSamplers[0].Sample(
									            player->GetWalkAnimationProgress() * 2.5f) *
									          0.005f * sp;
									vibPitch += coherentNoiseSamplers[1].Sample(
									              player->GetWalkAnimationProgress() * 2.5f) *
									            0.01f * sp;
									roll += coherentNoiseSamplers[2].Sample(
									          player->GetWalkAnimationProgress() * 2.5f) *
									        0.008f * sp;

									scale += sp * 0.1f;
								}
							}
						}

						scale /= GetAimDownZoomScale();

						def.fovY = (float)cg_fov * static_cast<float>(M_PI) / 180.f;
						def.fovX = atanf(tanf(def.fovY * .5f) * renderer->ScreenWidth() /
						                 renderer->ScreenHeight()) *
						           2.f;

						// for 1st view, camera blur can be used
						def.denyCameraBlur = false;

						float aimDownState = GetAimDownState();
						float per = aimDownState;
						per *= per * per;
						def.depthOfFieldFocalLength = per * 13.f + .054f;

						def.blurVignette = .0f;
					}

					// add vibration for both 1st/3rd view
					{
						// add grenade vibration
						float grenVib = grenadeVibration;
						if (grenVib > 0.f) {
							if (shakeLevel >= 1) {
								grenVib *= 10.f;
								if (grenVib > 1.f)
									grenVib = 1.f;
								roll += (nextRandom() - nextRandom()) * 0.2f * grenVib;
								vibPitch += (nextRandom() - nextRandom()) * 0.1f * grenVib;
								vibYaw += (nextRandom() - nextRandom()) * 0.1f * grenVib;
								scale -= (nextRandom() - nextRandom()) * 0.1f * grenVib;

								def.radialBlur += grenVib * 0.1f;
							}
						}
					}
					{
						// add grenade vibration
						float grenVib = grenadeVibrationSlow;
						if (grenVib > 0.f) {
							if (shakeLevel >= 2) {
								grenVib *= 4.f;
								if (grenVib > 1.f)
									grenVib = 1.f;
								grenVib *= grenVib;

								roll +=
								  coherentNoiseSamplers[0].Sample(time * 8.f) * 0.2f * grenVib;
								vibPitch +=
								  coherentNoiseSamplers[1].Sample(time * 12.f) * 0.1f * grenVib;
								vibYaw +=
								  coherentNoiseSamplers[2].Sample(time * 11.f) * 0.1f * grenVib;
							}
						}
					}

					// add roll / scale
					{
						Vector3 right = def.viewAxis[0];
						Vector3 up = def.viewAxis[1];

						def.viewAxis[0] = right * cosf(roll) - up * sinf(roll);
						def.viewAxis[1] = up * cosf(roll) + right * sinf(roll);

						def.fovX = atanf(tanf(def.fovX * .5f) * scale) * 2.f;
						def.fovY = atanf(tanf(def.fovY * .5f) * scale) * 2.f;
					}
					{
						Vector3 u = def.viewAxis[1];
						Vector3 v = def.viewAxis[2];

						def.viewAxis[1] = u * cosf(vibPitch) - v * sinf(vibPitch);
						def.viewAxis[2] = v * cosf(vibPitch) + u * sinf(vibPitch);
					}
					{
						Vector3 u = def.viewAxis[0];
						Vector3 v = def.viewAxis[2];

						def.viewAxis[0] = u * cosf(vibYaw) - v * sinf(vibYaw);
						def.viewAxis[2] = v * cosf(vibYaw) + u * sinf(vibYaw);
					}
					{
						float wTime = world->GetTime();
						if (wTime < lastHurtTime + .15f && wTime >= lastHurtTime) {
							float per = 1.f - (wTime - lastHurtTime) / .15f;
							per *= .5f - player->GetHealth() / 100.f * .3f;
							def.blurVignette += per * 6.f;
						}
						if (wTime < lastHurtTime + .2f && wTime >= lastHurtTime) {
							float per = 1.f - (wTime - lastHurtTime) / .2f;
							per *= .5f - player->GetHealth() / 100.f * .3f;
							def.saturation *= std::max(0.f, 1.f - per * 4.f);
						}
					}

					def.zNear = 0.05f;

					def.skipWorld = false;
				} else {
					def.viewOrigin = MakeVector3(256, 256, 4);
					def.viewAxis[0] = MakeVector3(-1, 0, 0);
					def.viewAxis[1] = MakeVector3(0, 1, 0);
					def.viewAxis[2] = MakeVector3(0, 0, 1);

					def.fovY = (float)cg_fov * static_cast<float>(M_PI) / 180.f;
					def.fovX = atanf(tanf(def.fovY * .5f) * renderer->ScreenWidth() /
					                 renderer->ScreenHeight()) *
					           2.f;

					def.zNear = 0.05f;

					def.skipWorld = false;
				}

			} else {
				def.viewOrigin = MakeVector3(0, 0, 0);
				def.viewAxis[0] = MakeVector3(1, 0, 0);
				def.viewAxis[1] = MakeVector3(0, 0, -1);
				def.viewAxis[2] = MakeVector3(0, 0, 1);

				def.fovY = (float)cg_fov * static_cast<float>(M_PI) / 180.f;
				def.fovX =
				  atanf(tanf(def.fovY * .5f) * renderer->ScreenWidth() / renderer->ScreenHeight()) *
				  2.f;

				def.zNear = 0.05f;

				def.skipWorld = true;

				renderer->SetFogColor(MakeVector3(0, 0, 0));
			}

			if (def.viewOrigin.z < 0.f) {
				// Need to move the far plane because there's no vertical fog
				def.zFar -= def.viewOrigin.z;
			}

			SPAssert(!std::isnan(def.viewOrigin.x));
			SPAssert(!std::isnan(def.viewOrigin.y));
			SPAssert(!std::isnan(def.viewOrigin.z));

			def.radialBlur = std::min(def.radialBlur, 1.f);

			if ((int)cg_manualFocus) {
				// Depth of field is manually controlled
				def.depthOfFieldNearBlurStrength = def.depthOfFieldFarBlurStrength =
				  0.5f * (float)cg_depthOfFieldAmount;
				def.depthOfFieldFocalLength = focalLength;
			} else {
				def.depthOfFieldNearBlurStrength = cg_depthOfFieldAmount;
				def.depthOfFieldFarBlurStrength = 0.f;
			}

			return def;
		}

		void Client::AddGrenadeToScene(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();

			IModel *model;
			model = renderer->RegisterModel("Models/Weapons/Grenade/Grenade.kv6");

			if (g->GetPosition().z > 63.f) {
				// work-around for water refraction problem
				return;
			}

			// Move the grenade slightly so that it doesn't look like sinking in
			// the ground
			Vector3 position = g->GetPosition();
			position.z -= 0.03f * 3.0f;

			ModelRenderParam param;
			Matrix4 mat = g->GetOrientation().ToRotationMatrix() * Matrix4::Scale(0.03f);
			mat = Matrix4::Translate(position) * mat;
			param.matrix = mat;

			renderer->RenderModel(model, param);
		}

		void Client::AddDebugObjectToScene(const spades::OBB3 &obb, const Vector4 &color) {
			const Matrix4 &mat = obb.m;
			Vector3 v[2][2][2];
			v[0][0][0] = (mat * MakeVector3(0, 0, 0)).GetXYZ();
			v[0][0][1] = (mat * MakeVector3(0, 0, 1)).GetXYZ();
			v[0][1][0] = (mat * MakeVector3(0, 1, 0)).GetXYZ();
			v[0][1][1] = (mat * MakeVector3(0, 1, 1)).GetXYZ();
			v[1][0][0] = (mat * MakeVector3(1, 0, 0)).GetXYZ();
			v[1][0][1] = (mat * MakeVector3(1, 0, 1)).GetXYZ();
			v[1][1][0] = (mat * MakeVector3(1, 1, 0)).GetXYZ();
			v[1][1][1] = (mat * MakeVector3(1, 1, 1)).GetXYZ();

			renderer->AddDebugLine(v[0][0][0], v[1][0][0], color);
			renderer->AddDebugLine(v[0][0][1], v[1][0][1], color);
			renderer->AddDebugLine(v[0][1][0], v[1][1][0], color);
			renderer->AddDebugLine(v[0][1][1], v[1][1][1], color);

			renderer->AddDebugLine(v[0][0][0], v[0][1][0], color);
			renderer->AddDebugLine(v[0][0][1], v[0][1][1], color);
			renderer->AddDebugLine(v[1][0][0], v[1][1][0], color);
			renderer->AddDebugLine(v[1][0][1], v[1][1][1], color);

			renderer->AddDebugLine(v[0][0][0], v[0][0][1], color);
			renderer->AddDebugLine(v[0][1][0], v[0][1][1], color);
			renderer->AddDebugLine(v[1][0][0], v[1][0][1], color);
			renderer->AddDebugLine(v[1][1][0], v[1][1][1], color);
		}

		void Client::DrawCTFObjects() {
			SPADES_MARK_FUNCTION();
			CTFGameMode *mode = static_cast<CTFGameMode *>(world->GetMode());
			int tId;
			IModel *base = renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			IModel *intel = renderer->RegisterModel("Models/MapObjects/Intel.kv6");
			for (tId = 0; tId < 2; tId++) {
				CTFGameMode::Team &team = mode->GetTeam(tId);
				IntVector3 col = world->GetTeam(tId).color;
				Vector3 color = {col.x / 255.f, col.y / 255.f, col.z / 255.f};

				ModelRenderParam param;
				param.customColor = color;

				// draw base
				param.matrix = Matrix4::Translate(team.basePos);
				param.matrix = param.matrix * Matrix4::Scale(.3f);
				renderer->RenderModel(base, param);

				// draw flag
				if (!mode->GetTeam(1 - tId).hasIntel) {
					param.matrix = Matrix4::Translate(team.flagPos);
					param.matrix = param.matrix * Matrix4::Scale(.1f);
					renderer->RenderModel(intel, param);
				}
			}
		}

		void Client::DrawTCObjects() {
			SPADES_MARK_FUNCTION();
			TCGameMode *mode = static_cast<TCGameMode *>(world->GetMode());
			int tId;
			IModel *base = renderer->RegisterModel("Models/MapObjects/CheckPoint.kv6");
			int cnt = mode->GetNumTerritories();
			for (tId = 0; tId < cnt; tId++) {
				TCGameMode::Territory *t = mode->GetTerritory(tId);
				IntVector3 col;
				if (t->ownerTeamId == 2) {
					col = IntVector3::Make(255, 255, 255);
				} else {
					col = world->GetTeam(t->ownerTeamId).color;
				}
				Vector3 color = {col.x / 255.f, col.y / 255.f, col.z / 255.f};

				ModelRenderParam param;
				param.customColor = color;

				// draw base
				param.matrix = Matrix4::Translate(t->pos);
				param.matrix = param.matrix * Matrix4::Scale(.3f);
				renderer->RenderModel(base, param);
			}
		}

		void Client::DrawScene() {
			SPADES_MARK_FUNCTION();

			renderer->StartScene(lastSceneDef);

			if (world) {
				Player *p = world->GetLocalPlayer();

				for (size_t i = 0; i < world->GetNumPlayerSlots(); i++)
					if (world->GetPlayer(static_cast<unsigned int>(i))) {
						SPAssert(clientPlayers[i]);
						clientPlayers[i]->AddToScene();
					}
				std::vector<Grenade *> nades = world->GetAllGrenades();
				for (size_t i = 0; i < nades.size(); i++) {
					AddGrenadeToScene(nades[i]);
				}

				{
					for (auto &c : corpses) {
						Vector3 center = c->GetCenter();
						if ((center - lastSceneDef.viewOrigin).GetPoweredLength() > 150.f * 150.f)
							continue;
						c->AddToScene();
					}
				}

				if (IGameMode::m_CTF == world->GetMode()->ModeType()) {
					DrawCTFObjects();
				} else if (IGameMode::m_TC == world->GetMode()->ModeType()) {
					DrawTCObjects();
				}

				{
					for (auto &ent : localEntities) {
						ent->Render3D();
					}
				}

				// draw block cursor
				// FIXME: don't use debug line
				if (p) {
					if (p->IsReadyToUseTool() && p->GetTool() == Player::ToolBlock &&
					    p->IsAlive()) {
						std::vector<IntVector3> blocks;
						if (p->IsBlockCursorDragging()) {
							blocks = std::move(world->CubeLine(p->GetBlockCursorDragPos(),
							                                   p->GetBlockCursorPos(), 256));
						} else {
							blocks.push_back(p->GetBlockCursorPos());
						}

						bool active = p->IsBlockCursorActive();

						Vector4 color = {1, 1, 1, 1};
						if (!active)
							color = Vector4(1, 1, 0, 1);
						if ((int)blocks.size() > p->GetNumBlocks())
							color = MakeVector4(1, 0, 0, 1);
						if (!active)
							color.w *= 0.5f;

						for (size_t i = 0; i < blocks.size(); i++) {
							IntVector3 &v = blocks[i];

							if (active) {

								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
								                       MakeVector3(v.x + 1, v.y, v.z), color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y + 1, v.z),
								                       MakeVector3(v.x + 1, v.y + 1, v.z), color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z + 1),
								                       MakeVector3(v.x + 1, v.y, v.z + 1), color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y + 1, v.z + 1),
								                       MakeVector3(v.x + 1, v.y + 1, v.z + 1),
								                       color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
								                       MakeVector3(v.x + 1, v.y, v.z), color);

								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
								                       MakeVector3(v.x, v.y + 1, v.z), color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z + 1),
								                       MakeVector3(v.x, v.y + 1, v.z + 1), color);
								renderer->AddDebugLine(MakeVector3(v.x + 1, v.y, v.z),
								                       MakeVector3(v.x + 1, v.y + 1, v.z), color);
								renderer->AddDebugLine(MakeVector3(v.x + 1, v.y, v.z + 1),
								                       MakeVector3(v.x + 1, v.y + 1, v.z + 1),
								                       color);

								renderer->AddDebugLine(MakeVector3(v.x, v.y, v.z),
								                       MakeVector3(v.x, v.y, v.z + 1), color);
								renderer->AddDebugLine(MakeVector3(v.x, v.y + 1, v.z),
								                       MakeVector3(v.x, v.y + 1, v.z + 1), color);
								renderer->AddDebugLine(MakeVector3(v.x + 1, v.y, v.z),
								                       MakeVector3(v.x + 1, v.y, v.z + 1), color);
								renderer->AddDebugLine(MakeVector3(v.x + 1, v.y + 1, v.z),
								                       MakeVector3(v.x + 1, v.y + 1, v.z + 1),
								                       color);
							} else {
								// not active

								const float ln = 0.1f;
								{
									float xx = v.x, yy = v.y, zz = v.z;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx + ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy + ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz + ln), color);
								}
								{
									float xx = v.x + 1, yy = v.y, zz = v.z;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx - ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy + ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz + ln), color);
								}
								{
									float xx = v.x, yy = v.y + 1, zz = v.z;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx + ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy - ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz + ln), color);
								}
								{
									float xx = v.x + 1, yy = v.y + 1, zz = v.z;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx - ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy - ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz + ln), color);
								}
								{
									float xx = v.x, yy = v.y, zz = v.z + 1;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx + ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy + ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz - ln), color);
								}
								{
									float xx = v.x + 1, yy = v.y, zz = v.z + 1;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx - ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy + ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz - ln), color);
								}
								{
									float xx = v.x, yy = v.y + 1, zz = v.z + 1;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx + ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy - ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz - ln), color);
								}
								{
									float xx = v.x + 1, yy = v.y + 1, zz = v.z + 1;
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx - ln, yy, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy - ln, zz), color);
									renderer->AddDebugLine(Vector3(xx, yy, zz),
									                       Vector3(xx, yy, zz - ln), color);
								}
							}
							// --- one block drawn
						} // end for

					} // p->IsReadyToUseTool
				}
			}

			for (size_t i = 0; i < flashDlights.size(); i++)
				renderer->AddLight(flashDlights[i]);
			flashDlightsOld.clear();
			flashDlightsOld.swap(flashDlights);

			// draw player hottrack
			// FIXME: don't use debug line
			{
				hitTag_t tag = hit_None;
				Player *hottracked = HotTrackedPlayer(&tag);
				if (hottracked) {
					IntVector3 col = world->GetTeam(hottracked->GetTeamId()).color;
					Vector4 color = Vector4::Make(col.x / 255.f, col.y / 255.f, col.z / 255.f, 1.f);
					Vector4 color2 = Vector4::Make(1, 1, 1, 1);

					Player::HitBoxes hb = hottracked->GetHitBoxes();
					AddDebugObjectToScene(hb.head, (tag & hit_Head) ? color2 : color);
					AddDebugObjectToScene(hb.torso, (tag & hit_Torso) ? color2 : color);
					AddDebugObjectToScene(hb.limbs[0], (tag & hit_Legs) ? color2 : color);
					AddDebugObjectToScene(hb.limbs[1], (tag & hit_Legs) ? color2 : color);
					AddDebugObjectToScene(hb.limbs[2], (tag & hit_Arms) ? color2 : color);
				}
			}

			renderer->EndScene();
		}

		Vector3 Client::Project(spades::Vector3 v) {
			v -= lastSceneDef.viewOrigin;

			// transform to NDC
			Vector3 v2;
			v2.x = Vector3::Dot(v, lastSceneDef.viewAxis[0]);
			v2.y = Vector3::Dot(v, lastSceneDef.viewAxis[1]);
			v2.z = Vector3::Dot(v, lastSceneDef.viewAxis[2]);

			float tanX = tanf(lastSceneDef.fovX * .5f);
			float tanY = tanf(lastSceneDef.fovY * .5f);

			v2.x /= tanX * v2.z;
			v2.y /= tanY * v2.z;

			// transform to IRenderer 2D coord
			v2.x = (v2.x + 1.f) / 2.f * renderer->ScreenWidth();
			v2.y = (-v2.y + 1.f) / 2.f * renderer->ScreenHeight();

			return v2;
		}
	}
}
