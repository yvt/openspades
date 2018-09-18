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

#include <cmath>
#include <cstdlib>
#include <iterator>

#include "Client.h"

#include <Core/ConcurrentDispatch.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

#include "IAudioChunk.h"
#include "IAudioDevice.h"

#include "CenterMessageView.h"
#include "ChatWindow.h"
#include "ClientPlayer.h"
#include "ClientUI.h"
#include "Corpse.h"
#include "FallingBlock.h"
#include "HurtRingView.h"
#include "ILocalEntity.h"
#include "LimboView.h"
#include "MapView.h"
#include "PaletteView.h"
#include "ParticleSpriteEntity.h"
#include "SmokeSpriteEntity.h"

#include "GameMap.h"
#include "Grenade.h"
#include "Weapon.h"
#include "World.h"

#include "NetClient.h"

DEFINE_SPADES_SETTING(cg_blood, "1");
DEFINE_SPADES_SETTING(cg_particles, "2");
DEFINE_SPADES_SETTING(cg_waterImpact, "1");
SPADES_SETTING(cg_manualFocus);
DEFINE_SPADES_SETTING(cg_autoFocusSpeed, "0.4");

namespace spades {
	namespace client {

#pragma mark - Local Entities / Effects

		void Client::RemoveAllCorpses() {
			SPADES_MARK_FUNCTION();

			corpses.clear();
			lastMyCorpse = nullptr;
		}

		void Client::RemoveAllLocalEntities() {
			SPADES_MARK_FUNCTION();

			localEntities.clear();
		}

		void Client::RemoveInvisibleCorpses() {
			SPADES_MARK_FUNCTION();

			decltype(corpses)::iterator it;
			std::vector<decltype(it)> its;
			int cnt = (int)corpses.size() - corpseSoftLimit;
			for (it = corpses.begin(); it != corpses.end(); it++) {
				if (cnt <= 0)
					break;
				auto &c = *it;
				if (!c->IsVisibleFrom(lastSceneDef.viewOrigin)) {
					if (c.get() == lastMyCorpse)
						lastMyCorpse = nullptr;
					its.push_back(it);
				}
				cnt--;
			}

			for (size_t i = 0; i < its.size(); i++)
				corpses.erase(its[i]);
		}

		void Client::RemoveCorpseForPlayer(int playerId) {
			for (auto it = corpses.begin(); it != corpses.end();) {
				auto cur = it;
				++it;

				auto &c = *cur;
				if (c->GetPlayerId() == playerId) {
					corpses.erase(cur);
				}
			}
		}

		Player *Client::HotTrackedPlayer(hitTag_t *hitFlag) {
			if (!IsFirstPerson(GetCameraMode()))
				return nullptr;

			auto &p = GetCameraTargetPlayer();

			Vector3 origin = p.GetEye();
			Vector3 dir = p.GetFront();
			World::WeaponRayCastResult result = world->WeaponRayCast(origin, dir, &p);

			if (result.hit == false || result.player == nullptr)
				return nullptr;

			// don't hot track enemies (non-spectator only)
			if (result.player->GetTeamId() != p.GetTeamId() && p.GetTeamId() < 2)
				return nullptr;
			if (hitFlag) {
				*hitFlag = result.hitFlag;
			}
			return result.player;
		}

		bool Client::IsMuted() {
			// prevent to play loud sound at connection
			// caused by saved packets
			return time < worldSetTime + .05f;
		}

		void Client::Bleed(spades::Vector3 v) {
			SPADES_MARK_FUNCTION();

			if (!cg_blood)
				return;

			// distance cull
			if ((v - lastSceneDef.viewOrigin).GetPoweredLength() > 150.f * 150.f)
				return;

			if ((int)cg_particles < 1)
				return;
            
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {0.5f, 0.02f, 0.04f, 1.f};
			for (int i = 0; i < 10; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(v,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     10.f,
				                   1.f, 0.7f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(0.1f + SampleRandomFloat() * SampleRandomFloat() * 0.2f);
				ent->SetLifeTime(3.f, 0.f, 1.f);
				localEntities.emplace_back(ent);
			}

			if ((int)cg_particles < 2)
				return;

			color = MakeVector4(.7f, .35f, .37f, .6f);
			for (int i = 0; i < 2; i++) {
				ParticleSpriteEntity *ent =
				  new SmokeSpriteEntity(this, color, 100.f, SmokeSpriteEntity::Type::Explosion);
				ent->SetTrajectory(v,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     .7f,
				                   .8f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.5f + SampleRandomFloat() * SampleRandomFloat() * 0.2f, 2.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.20f + SampleRandomFloat() * .2f, 0.06f, .20f);
				localEntities.emplace_back(ent);
			}

			color.w *= .1f;
			for (int i = 0; i < 1; i++) {
				ParticleSpriteEntity *ent =
				  new SmokeSpriteEntity(this, color, 40.f, SmokeSpriteEntity::Type::Steady);
				ent->SetTrajectory(v,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     .7f,
				                   .8f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.7f + SampleRandomFloat() * SampleRandomFloat() * 0.2f, 2.f, 0.1f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.80f + SampleRandomFloat() * 0.4f, 0.06f, 1.0f);
				localEntities.emplace_back(ent);
			}
		}

		void Client::EmitBlockFragments(Vector3 origin, IntVector3 c) {
			SPADES_MARK_FUNCTION();

			// distance cull
			float distPowered = (origin - lastSceneDef.viewOrigin).GetPoweredLength();
			if (distPowered > 150.f * 150.f)
				return;

			if ((int)cg_particles < 1)
				return;

			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {c.x / 255.f, c.y / 255.f, c.z / 255.f, 1.f};
			for (int i = 0; i < 7; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     7.f,
				                   1.f, .9f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(0.2f + SampleRandomFloat() * SampleRandomFloat() * 0.1f);
				ent->SetLifeTime(2.f, 0.f, 1.f);
				if (distPowered < 16.f * 16.f)
					ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.emplace_back(ent);
			}

			if ((int)cg_particles < 2)
				return;

			if (distPowered < 32.f * 32.f) {
				for (int i = 0; i < 16; i++) {
					ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
					ent->SetTrajectory(origin, MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
					                                       SampleRandomFloat() - SampleRandomFloat(),
					                                       SampleRandomFloat() - SampleRandomFloat()) *
					                             12.f,
					                   1.f, .9f);
					ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
					ent->SetRadius(0.1f + SampleRandomFloat() * SampleRandomFloat() * 0.14f);
					ent->SetLifeTime(2.f, 0.f, 1.f);
					if (distPowered < 16.f * 16.f)
						ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
					localEntities.emplace_back(ent);
				}
			}

			color += (MakeVector4(1, 1, 1, 1) - color) * .2f;
			color.w *= .2f;
			for (int i = 0; i < 2; i++) {
				ParticleSpriteEntity *ent = new SmokeSpriteEntity(this, color, 100.f);
				ent->SetTrajectory(origin,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     .7f,
				                   1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.6f + SampleRandomFloat() * SampleRandomFloat() * 0.2f, 0.8f);
				ent->SetLifeTime(.3f + SampleRandomFloat() * .3f, 0.06f, .4f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				localEntities.emplace_back(ent);
			}
		}

		void Client::EmitBlockDestroyFragments(IntVector3 blk, IntVector3 c) {
			SPADES_MARK_FUNCTION();

			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};

			// distance cull
			if ((origin - lastSceneDef.viewOrigin).GetPoweredLength() > 150.f * 150.f)
				return;

			if ((int)cg_particles < 1)
				return;

			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			Vector4 color = {c.x / 255.f, c.y / 255.f, c.z / 255.f, 1.f};
			for (int i = 0; i < 8; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                               SampleRandomFloat() - SampleRandomFloat(),
				                               SampleRandomFloat() - SampleRandomFloat()) *
				                     7.f,
				                   1.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(0.3f + SampleRandomFloat() * SampleRandomFloat() * 0.2f);
				ent->SetLifeTime(2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.emplace_back(ent);
			}
		}

		void Client::MuzzleFire(spades::Vector3 origin, spades::Vector3 dir, bool local) {
			DynamicLightParam l;
			l.origin = origin;
			l.radius = 5.f;
			l.type = DynamicLightTypePoint;
			l.color = MakeVector3(3.f, 1.6f, 0.5f);
			flashDlights.push_back(l);

			if ((int)cg_particles < 1)
				return;

			Vector4 color;
			Vector3 velBias = {0, 0, -0.5f};
			color = MakeVector4(.8f, .8f, .8f, .3f);

			// rapid smoke
			for (int i = 0; i < 2; i++) {
				ParticleSpriteEntity *ent =
				  new SmokeSpriteEntity(this, color, 120.f, SmokeSpriteEntity::Type::Explosion);
				ent->SetTrajectory(
				  origin, (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                       SampleRandomFloat() - SampleRandomFloat(),
				                       SampleRandomFloat() - SampleRandomFloat()) +
				           velBias * .5f) *
				            0.3f,
				  1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.4f, 3.f, 0.0000005f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(0.2f + SampleRandomFloat() * 0.1f, 0.f, .30f);
				localEntities.emplace_back(ent);
			}
		}

		void Client::KickCamera(float strength) {
			grenadeVibration = std::min(grenadeVibration + strength, 0.4f);
			grenadeVibrationSlow = std::min(grenadeVibrationSlow + strength * 5.f, 0.4f);
		}

		void Client::GrenadeExplosion(spades::Vector3 origin) {
			float dist = (origin - lastSceneDef.viewOrigin).GetLength();
			if (dist > 170.f)
				return;
			KickCamera(2.f / (dist + 5.f));

			DynamicLightParam l;
			l.origin = origin;
			l.radius = 16.f;
			l.type = DynamicLightTypePoint;
			l.color = MakeVector3(3.f, 1.6f, 0.5f);
			l.useLensFlare = true;
			flashDlights.push_back(l);

			if ((int)cg_particles < 1)
				return;

			Vector3 velBias = {0, 0, 0};
			if (!map->ClipBox(origin.x, origin.y, origin.z)) {
				if (map->ClipBox(origin.x + 1.f, origin.y, origin.z)) {
					velBias.x -= 1.f;
				}
				if (map->ClipBox(origin.x - 1.f, origin.y, origin.z)) {
					velBias.x += 1.f;
				}
				if (map->ClipBox(origin.x, origin.y + 1.f, origin.z)) {
					velBias.y -= 1.f;
				}
				if (map->ClipBox(origin.x, origin.y - 1.f, origin.z)) {
					velBias.y += 1.f;
				}
				if (map->ClipBox(origin.x, origin.y, origin.z + 1.f)) {
					velBias.z -= 1.f;
				}
				if (map->ClipBox(origin.x, origin.y, origin.z - 1.f)) {
					velBias.z += 1.f;
				}
			}

			Vector4 color;
			color = MakeVector4(.6f, .6f, .6f, 1.f);
			// rapid smoke
			for (int i = 0; i < 4; i++) {
				ParticleSpriteEntity *ent =
				  new SmokeSpriteEntity(this, color, 60.f, SmokeSpriteEntity::Type::Explosion);
				ent->SetTrajectory(
				  origin, (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                       SampleRandomFloat() - SampleRandomFloat(),
				                       SampleRandomFloat() - SampleRandomFloat()) +
				           velBias * .5f) *
				            2.f,
				  1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.6f + SampleRandomFloat() * SampleRandomFloat() * 0.4f, 2.f, .2f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(1.8f + SampleRandomFloat() * 0.1f, 0.f, .20f);
				localEntities.emplace_back(ent);
			}

			// slow smoke
			color.w = .25f;
			for (int i = 0; i < 8; i++) {
				ParticleSpriteEntity *ent = new SmokeSpriteEntity(this, color, 20.f);
				ent->SetTrajectory(
				  origin, (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                       SampleRandomFloat() - SampleRandomFloat(),
				                       (SampleRandomFloat() - SampleRandomFloat()) * .2f)) *
				            2.f,
				  1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(1.5f + SampleRandomFloat() * SampleRandomFloat() * 0.8f, 0.2f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				switch ((int)cg_particles) {
					case 1: ent->SetLifeTime(0.8f + SampleRandomFloat() * 1.f, 0.1f, 8.f); break;
					case 2: ent->SetLifeTime(1.5f + SampleRandomFloat() * 2.f, 0.1f, 8.f); break;
					case 3:
					default: ent->SetLifeTime(2.f + SampleRandomFloat() * 5.f, 0.1f, 8.f); break;
				}
				localEntities.emplace_back(ent);
			}

			// fragments
			Handle<IImage> img = renderer->RegisterImage("Gfx/White.tga");
			color = MakeVector4(0.01, 0.03, 0, 1.f);
			for (int i = 0; i < 42; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				Vector3 dir = MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                          SampleRandomFloat() - SampleRandomFloat(),
				                          SampleRandomFloat() - SampleRandomFloat());
				dir += velBias * .5f;
				float radius = 0.1f + SampleRandomFloat() * SampleRandomFloat() * 0.2f;
				ent->SetTrajectory(origin + dir * .2f, dir * 20.f, .1f + radius * 3.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(radius);
				ent->SetLifeTime(3.5f + SampleRandomFloat() * 2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::BounceWeak);
				localEntities.emplace_back(ent);
			}

			// fire smoke
			color = MakeVector4(1.f, .7f, .4f, .2f) * 5.f;
			for (int i = 0; i < 4; i++) {
				ParticleSpriteEntity *ent =
				  new SmokeSpriteEntity(this, color, 120.f, SmokeSpriteEntity::Type::Explosion);
				ent->SetTrajectory(
				  origin, (MakeVector3(SampleRandomFloat() - SampleRandomFloat(), SampleRandomFloat() - SampleRandomFloat(),
				                       SampleRandomFloat() - SampleRandomFloat()) +
				           velBias) *
				            6.f,
				  1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(.3f + SampleRandomFloat() * SampleRandomFloat() * 0.4f, 3.f, .1f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(.18f + SampleRandomFloat() * 0.03f, 0.f, .10f);
				// ent->SetAdditive(true);
				localEntities.emplace_back(ent);
			}
		}

		void Client::GrenadeExplosionUnderwater(spades::Vector3 origin) {
			float dist = (origin - lastSceneDef.viewOrigin).GetLength();
			if (dist > 170.f)
				return;
			KickCamera(1.5f / (dist + 5.f));

			if ((int)cg_particles < 1)
				return;

			Vector3 velBias = {0, 0, 0};

			Vector4 color;
			color = MakeVector4(.95f, .95f, .95f, .6f);
			// water1
			Handle<IImage> img = renderer->RegisterImage("Textures/WaterExpl.png");
			if ((int)cg_particles < 2)
				color.w = .3f;
			for (int i = 0; i < 7; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
				                                SampleRandomFloat() - SampleRandomFloat(), -SampleRandomFloat() * 7.f)) *
				                     2.5f,
				                   .3f, .6f);
				ent->SetRotation(0.f);
				ent->SetRadius(1.5f + SampleRandomFloat() * SampleRandomFloat() * 0.4f, 1.3f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + SampleRandomFloat() * 0.3f, 0.f, .60f);
				localEntities.emplace_back(ent);
			}

			// water2
			img = renderer->RegisterImage("Textures/Fluid.png");
			color.w = .9f;
			if ((int)cg_particles < 2)
				color.w = .4f;
			for (int i = 0; i < 16; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
				                                SampleRandomFloat() - SampleRandomFloat(), -SampleRandomFloat() * 10.f)) *
				                     3.5f,
				                   1.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(0.9f + SampleRandomFloat() * SampleRandomFloat() * 0.4f, 0.7f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + SampleRandomFloat() * 0.3f, .7f, .60f);
				localEntities.emplace_back(ent);
			}

			// slow smoke
			color.w = .4f;
			if ((int)cg_particles < 2)
				color.w = .2f;
			for (int i = 0; i < 8; i++) {
				ParticleSpriteEntity *ent = new SmokeSpriteEntity(this, color, 20.f);
				ent->SetTrajectory(
				  origin, (MakeVector3(SampleRandomFloat() - SampleRandomFloat(), SampleRandomFloat() - SampleRandomFloat(),
				                       (SampleRandomFloat() - SampleRandomFloat()) * .2f)) *
				            2.f,
				  1.f, 0.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(1.4f + SampleRandomFloat() * SampleRandomFloat() * 0.8f, 0.2f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				switch ((int)cg_particles) {
					case 1: ent->SetLifeTime(3.f + SampleRandomFloat() * 5.f, 0.1f, 8.f); break;
					case 2:
					case 3:
					default: ent->SetLifeTime(6.f + SampleRandomFloat() * 5.f, 0.1f, 8.f); break;
				}
				localEntities.emplace_back(ent);
			}

			// fragments
			img = renderer->RegisterImage("Gfx/White.tga");
			color = MakeVector4(1, 1, 1, 0.7f);
			for (int i = 0; i < 42; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				Vector3 dir = MakeVector3(SampleRandomFloat() - SampleRandomFloat(), SampleRandomFloat() - SampleRandomFloat(),
				                          -SampleRandomFloat() * 3.f);
				dir += velBias * .5f;
				float radius = 0.1f + SampleRandomFloat() * SampleRandomFloat() * 0.2f;
				ent->SetTrajectory(origin + dir * .2f + MakeVector3(0, 0, -1.2f), dir * 13.f,
				                   .1f + radius * 3.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(radius);
				ent->SetLifeTime(3.5f + SampleRandomFloat() * 2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Delete);
				localEntities.emplace_back(ent);
			}

			// TODO: wave?
		}

		void Client::BulletHitWaterSurface(spades::Vector3 origin) {
			float dist = (origin - lastSceneDef.viewOrigin).GetLength();
			if (dist > 150.f)
				return;
			if (!cg_waterImpact)
				return;

			if ((int)cg_particles < 1)
				return;
            
			Vector4 color;
			color = MakeVector4(.95f, .95f, .95f, .3f);
			// water1
			Handle<IImage> img = renderer->RegisterImage("Textures/WaterExpl.png");
			if ((int)cg_particles < 2)
				color.w = .2f;
			for (int i = 0; i < 2; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
				                                SampleRandomFloat() - SampleRandomFloat(), -SampleRandomFloat() * 7.f)) *
				                     1.f,
				                   .3f, .6f);
				ent->SetRotation(0.f);
				ent->SetRadius(0.6f + SampleRandomFloat() * SampleRandomFloat() * 0.4f, .7f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + SampleRandomFloat() * 0.3f, 0.1f, .60f);
				localEntities.emplace_back(ent);
			}

			// water2
			img = renderer->RegisterImage("Textures/Fluid.png");
			color.w = .9f;
			if ((int)cg_particles < 2)
				color.w = .4f;
			for (int i = 0; i < 6; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				ent->SetTrajectory(origin,
				                   (MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
				                                SampleRandomFloat() - SampleRandomFloat(), -SampleRandomFloat() * 10.f)) *
				                     2.f,
				                   1.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(0.6f + SampleRandomFloat() * SampleRandomFloat() * 0.6f, 0.6f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Ignore);
				ent->SetLifeTime(3.f + SampleRandomFloat() * 0.3f, SampleRandomFloat() * 0.3f, .60f);
				localEntities.emplace_back(ent);
			}

			// fragments
			img = renderer->RegisterImage("Gfx/White.tga");
			color = MakeVector4(1, 1, 1, 0.7f);
			for (int i = 0; i < 10; i++) {
				ParticleSpriteEntity *ent = new ParticleSpriteEntity(this, img, color);
				Vector3 dir = MakeVector3(SampleRandomFloat() - SampleRandomFloat(),
                                          SampleRandomFloat() - SampleRandomFloat(),
				                          -SampleRandomFloat() * 3.f);
				float radius = 0.03f + SampleRandomFloat() * SampleRandomFloat() * 0.05f;
				ent->SetTrajectory(origin + dir * .2f + MakeVector3(0, 0, -1.2f), dir * 5.f,
				                   .1f + radius * 3.f, 1.f);
				ent->SetRotation(SampleRandomFloat() * (float)M_PI * 2.f);
				ent->SetRadius(radius);
				ent->SetLifeTime(3.5f + SampleRandomFloat() * 2.f, 0.f, 1.f);
				ent->SetBlockHitAction(ParticleSpriteEntity::Delete);
				localEntities.emplace_back(ent);
			}

			// TODO: wave?
		}

#pragma mark - Camera Control

		enum { AutoFocusPoints = 4 };
		void Client::UpdateAutoFocus(float dt) {
			if (autoFocusEnabled && world && (int)cg_manualFocus) {
				// Compute focal length
				float measureRange = tanf(lastSceneDef.fovY * .5f) * .2f;
				const Vector3 camOrigin = lastSceneDef.viewOrigin;
				const float lenScale = 1.f / lastSceneDef.viewAxis[2].GetLength();
				const Vector3 camDir = lastSceneDef.viewAxis[2].Normalize();
				const Vector3 camX = lastSceneDef.viewAxis[0].Normalize() * measureRange;
				const Vector3 camY = lastSceneDef.viewAxis[1].Normalize() * measureRange;

				float distances[AutoFocusPoints * AutoFocusPoints];
				std::size_t numValidDistances = 0;
				Vector3 camDir1 = camDir - camX - camY;
				const Vector3 camDX = camX * (2.f / (AutoFocusPoints - 1));
				const Vector3 camDY = camY * (2.f / (AutoFocusPoints - 1));
				for (int x = 0; x < AutoFocusPoints; ++x) {
					Vector3 camDir2 = camDir1;
					for (int y = 0; y < AutoFocusPoints; ++y) {
						float dist = RayCastForAutoFocus(camOrigin, camDir2);

						dist *= lenScale;

						if (std::isfinite(dist) && dist > 0.8f) {
							distances[numValidDistances++] = dist;
						}

						camDir2 += camDY;
					}
					camDir1 += camDX;
				}

				if (numValidDistances > 0) {
					// Take median
					std::sort(distances, distances + numValidDistances);

					float dist = (numValidDistances & 1)
					               ? distances[numValidDistances >> 1]
					               : (distances[numValidDistances >> 1] +
					                  distances[(numValidDistances >> 1) - 1]) *
					                   0.5f;

					targetFocalLength = dist;
				}
			}

			// Change the actual focal length slowly
			{
				float dist = 1.f / targetFocalLength;
				float curDist = 1.f / focalLength;
				const float maxSpeed = cg_autoFocusSpeed;

				if (dist > curDist) {
					curDist = std::min(dist, curDist + maxSpeed * dt);
				} else {
					curDist = std::max(dist, curDist - maxSpeed * dt);
				}

				focalLength = 1.f / curDist;
			}
		}
		float Client::RayCastForAutoFocus(const Vector3 &origin, const Vector3 &direction) {
			SPAssert(world);

			const auto &lastSceneDef = this->lastSceneDef;
			World::WeaponRayCastResult result = world->WeaponRayCast(origin, direction, nullptr);
			if (result.hit) {
				return Vector3::Dot(result.hitPos - origin, lastSceneDef.viewAxis[2]);
			}

			return std::nan(nullptr);
		}
	}
}
