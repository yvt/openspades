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
#include "Tracer.h"

#include "GameMap.h"
#include "Grenade.h"
#include "Weapon.h"
#include "World.h"

#include "NetClient.h"

DEFINE_SPADES_SETTING(cg_ragdoll, "1");
SPADES_SETTING(cg_blood);
DEFINE_SPADES_SETTING(cg_ejectBrass, "1");
DEFINE_SPADES_SETTING(cg_hitFeedbackSoundGain, "0.2");

SPADES_SETTING(cg_alerts);
SPADES_SETTING(cg_centerMessage);

SPADES_SETTING(cg_shake);

SPADES_SETTING(cg_holdAimDownSight);

namespace spades {
	namespace client {

#pragma mark - World States

		float Client::GetSprintState() {
			if (!world)
				return 0.f;
			if (!world->GetLocalPlayer())
				return 0.f;

			ClientPlayer *p = clientPlayers[(int)world->GetLocalPlayerIndex()];
			if (!p)
				return 0.f;
			return p->GetSprintState();
		}

		float Client::GetAimDownState() {
			if (!world)
				return 0.f;
			if (!world->GetLocalPlayer())
				return 0.f;

			ClientPlayer *p = clientPlayers[(int)world->GetLocalPlayerIndex()];
			if (!p)
				return 0.f;
			return p->GetAimDownState();
		}

		bool Client::CanLocalPlayerUseToolNow() {
			if (!world || !world->GetLocalPlayer() || !world->GetLocalPlayer()->IsAlive()) {
				return false;
			}

			if (GetSprintState() > 0 || world->GetLocalPlayer()->GetInput().sprint) {
				// Player is unable to use a tool while/soon after sprinting
				return false;
			}

			auto *clientPlayer = GetLocalClientPlayer();
			SPAssert(clientPlayer);

			if (clientPlayer->IsChangingTool()) {
				// Player is unable to use a tool while switching to another tool
				return false;
			}

			return true;
		}

		ClientPlayer *Client::GetLocalClientPlayer() {
			if (!world || !world->GetLocalPlayer()) {
				return nullptr;
			}
			return clientPlayers.at(static_cast<std::size_t>(world->GetLocalPlayerIndex()));
		}

#pragma mark - World Actions
		/** Captures the color of the block player is looking at. */
		void Client::CaptureColor() {
			if (!world)
				return;
			Player *p = world->GetLocalPlayer();
			if (!p)
				return;
			if (!p->IsAlive())
				return;

			IntVector3 outBlockCoord;
			uint32_t col;
			if (!world->GetMap()->CastRay(p->GetEye(), p->GetFront(), 256.f, outBlockCoord)) {
				auto c = world->GetFogColor();
				col = c.x | c.y << 8 | c.z << 16;
			} else {
				col = world->GetMap()->GetColorWrapped(outBlockCoord.x, outBlockCoord.y,
				                                       outBlockCoord.z);
			}

			IntVector3 colV;
			colV.x = (uint8_t)(col);
			colV.y = (uint8_t)(col >> 8);
			colV.z = (uint8_t)(col >> 16);

			p->SetHeldBlockColor(colV);
			net->SendHeldBlockColor();
		}

		void Client::SetSelectedTool(Player::ToolType type, bool quiet) {
			if (type == world->GetLocalPlayer()->GetTool())
				return;
			lastTool = world->GetLocalPlayer()->GetTool();
			hasLastTool = true;

			world->GetLocalPlayer()->SetTool(type);
			net->SendTool();

			if (!quiet) {
				Handle<IAudioChunk> c =
				  audioDevice->RegisterSound("Sounds/Weapons/SwitchLocal.opus");
				audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
			}
		}

#pragma mark - World Update

		void Client::UpdateWorld(float dt) {
			SPADES_MARK_FUNCTION();

			Player *player = world->GetLocalPlayer();

			if (player) {

				// disable input when UI is open
				if (scriptedUI->NeedsInput()) {
					weapInput.primary = false;
					if (player->GetTeamId() >= 2 || player->GetTool() != Player::ToolWeapon) {
						weapInput.secondary = false;
					}
					playerInput = PlayerInput();
				}

				if (player->GetTeamId() >= 2) {
					UpdateLocalSpectator(dt);
				} else {
					UpdateLocalPlayer(dt);
				}
			}

#if 0
			// dynamic time step
			// physics diverges from server
			world->Advance(dt);
#else
			// accurately resembles server's physics
			// but not smooth
			if (dt > 0.f)
				worldSubFrame += dt;

			float frameStep = 1.f / 60.f;
			while (worldSubFrame >= frameStep) {
				world->Advance(frameStep);
				worldSubFrame -= frameStep;
			}
#endif

			// update player view (doesn't affect physics/game logics)
			for (size_t i = 0; i < clientPlayers.size(); i++) {
				if (clientPlayers[i]) {
					clientPlayers[i]->Update(dt);
				}
			}

			// corpse never accesses audio nor renderer, so
			// we can do it in the separate thread
			class CorpseUpdateDispatch : public ConcurrentDispatch {
				Client *client;
				float dt;

			public:
				CorpseUpdateDispatch(Client *c, float dt) : client(c), dt(dt) {}
				void Run() override {
					for (auto &c : client->corpses) {
						for (int i = 0; i < 4; i++)
							c->Update(dt / 4.f);
					}
				}
			};
			CorpseUpdateDispatch corpseDispatch(this, dt);
			corpseDispatch.Start();

			// local entities should be done in the client thread
			{
				decltype(localEntities)::iterator it;
				std::vector<decltype(it)> its;
				for (it = localEntities.begin(); it != localEntities.end(); it++) {
					if (!(*it)->Update(dt))
						its.push_back(it);
				}
				for (size_t i = 0; i < its.size(); i++) {
					localEntities.erase(its[i]);
				}
			}

			corpseDispatch.Join();

			if (grenadeVibration > 0.f) {
				grenadeVibration -= dt;
				if (grenadeVibration < 0.f)
					grenadeVibration = 0.f;
			}

			if (grenadeVibrationSlow > 0.f) {
				grenadeVibrationSlow -= dt;
				if (grenadeVibrationSlow < 0.f)
					grenadeVibrationSlow = 0.f;
			}

			if (hitFeedbackIconState > 0.f) {
				hitFeedbackIconState -= dt * 4.f;
				if (hitFeedbackIconState < 0.f)
					hitFeedbackIconState = 0.f;
			}

			if (time > lastPosSentTime + 1.f && world->GetLocalPlayer()) {
				Player *p = world->GetLocalPlayer();
				if (p->IsAlive() && p->GetTeamId() < 2) {
					net->SendPosition();
					lastPosSentTime = time;
				}
			}
		}

		/** Handles movement of spectating local player. */
		void Client::UpdateLocalSpectator(float dt) {
			SPADES_MARK_FUNCTION();

			auto &sharedState = followAndFreeCameraState;
			auto &freeState = freeCameraState;

			Vector3 lastPos = freeState.position;
			freeState.velocity *= powf(.3f, dt);
			freeState.position += freeState.velocity * dt;

			if (freeState.position.x < 0.f) {
				freeState.velocity.x = fabsf(freeState.velocity.x) * 0.2f;
				freeState.position = lastPos + freeState.velocity * dt;
			}
			if (freeState.position.y < 0.f) {
				freeState.velocity.y = fabsf(freeState.velocity.y) * 0.2f;
				freeState.position = lastPos + freeState.velocity * dt;
			}
			if (freeState.position.x > (float)GetWorld()->GetMap()->Width()) {
				freeState.velocity.x = fabsf(freeState.velocity.x) * -0.2f;
				freeState.position = lastPos + freeState.velocity * dt;
			}
			if (freeState.position.y > (float)GetWorld()->GetMap()->Height()) {
				freeState.velocity.y = fabsf(freeState.velocity.y) * -0.2f;
				freeState.position = lastPos + freeState.velocity * dt;
			}

			GameMap::RayCastResult minResult;
			float minDist = 1.e+10f;
			Vector3 minShift;

			// check collision
			if (freeState.velocity.GetLength() < .01) {
				freeState.position = lastPos;
				freeState.velocity *= 0.f;
			} else {
				for (int sx = -1; sx <= 1; sx++)
					for (int sy = -1; sy <= 1; sy++)
						for (int sz = -1; sz <= 1; sz++) {
							GameMap::RayCastResult result;
							Vector3 shift = {sx * .1f, sy * .1f, sz * .1f};
							result = map->CastRay2(lastPos + shift, freeState.position - lastPos, 256);
							if (result.hit && !result.startSolid &&
							    Vector3::Dot(result.hitPos - freeState.position - shift,
							                 freeState.position - lastPos) < 0.f) {

								float dist = Vector3::Dot(result.hitPos - freeState.position - shift,
								                          (freeState.position - lastPos).Normalize());
								if (dist < minDist) {
									minResult = result;
									minDist = dist;
									minShift = shift;
								}
							}
						}
			}
			if (minDist < 1.e+9f) {
				GameMap::RayCastResult result = minResult;
				Vector3 shift = minShift;
				freeState.position = result.hitPos - shift;
				freeState.position.x += result.normal.x * .02f;
				freeState.position.y += result.normal.y * .02f;
				freeState.position.z += result.normal.z * .02f;

				// bounce
				Vector3 norm = {(float)result.normal.x, (float)result.normal.y,
				                (float)result.normal.z};
				float dot = Vector3::Dot(freeState.velocity, norm);
				freeState.velocity -= norm * (dot * 1.2f);
			}

			// acceleration
			Vector3 front;
			Vector3 up = {0, 0, -1};

			front.x = -cosf(sharedState.yaw) * cosf(sharedState.pitch);
			front.y = -sinf(sharedState.yaw) * cosf(sharedState.pitch);
			front.z = sinf(sharedState.pitch);

			Vector3 right = -Vector3::Cross(up, front).Normalize();
			Vector3 up2 = Vector3::Cross(right, front).Normalize();

			float scale = 10.f * dt;
			if (playerInput.sprint) {
				scale *= 3.f;
			}
			front *= scale;
			right *= scale;
			up2 *= scale;

			if (playerInput.moveForward) {
				freeState.velocity += front;
			} else if (playerInput.moveBackward) {
				freeState.velocity -= front;
			}
			if (playerInput.moveLeft) {
				freeState.velocity -= right;
			} else if (playerInput.moveRight) {
				freeState.velocity += right;
			}
			if (playerInput.jump) {
				freeState.velocity += up2;
			} else if (playerInput.crouch) {
				freeState.velocity -= up2;
			}

			SPAssert(freeState.velocity.GetLength() < 100.f);
		}

		/** Handles movement of joined local player. */
		void Client::UpdateLocalPlayer(float dt) {
			SPADES_MARK_FUNCTION();

			auto *player = world->GetLocalPlayer();
			auto clientPlayer = clientPlayers[world->GetLocalPlayerIndex()];

			PlayerInput inp = playerInput;
			WeaponInput winp = weapInput;

			Vector3 velocity = player->GetVelocity();
			Vector3 horizontalVelocity{velocity.x, velocity.y, 0.0f};

			if (horizontalVelocity.GetLength() < 0.1f) {
				inp.sprint = false;
			}

			// Can't use a tool while sprinting or switching to another tool, etc.
			if (!CanLocalPlayerUseToolNow()) {
				winp.primary = false;
				winp.secondary = false;
			}

			// don't allow to stand up when ceilings are too low
			if (inp.crouch == false) {
				if (player->GetInput().crouch) {
					if (!player->TryUncrouch(false)) {
						inp.crouch = true;
					}
				}
			}

			// don't allow jumping in the air
			if (inp.jump) {
				if (!player->IsOnGroundOrWade())
					inp.jump = false;
			}

			if (player->GetTool() == Player::ToolWeapon) {
				// disable weapon while reloading (except shotgun)
				if (player->IsAwaitingReloadCompletion() && !player->GetWeapon()->IsReloadSlow()) {
					winp.primary = false;
					winp.secondary = false;
				}

				// stop firing if the player is out of ammo
				if (player->GetWeapon()->GetAmmo() == 0) {
					winp.primary = false;
				}
			}

			player->SetInput(inp);
			player->SetWeaponInput(winp);

			// send player input
			{
				PlayerInput sentInput = inp;
				WeaponInput sentWeaponInput = winp;

				// FIXME: send only there are any changed?
				net->SendPlayerInput(sentInput);
				net->SendWeaponInput(sentWeaponInput);
			}

			if (hasDelayedReload) {
				world->GetLocalPlayer()->Reload();
				net->SendReload();
				hasDelayedReload = false;
			}

			// PlayerInput actualInput = player->GetInput();
			WeaponInput actualWeapInput = player->GetWeaponInput();

			if (!(actualWeapInput.secondary && player->IsToolWeapon() && player->IsAlive()) &&
				!(cg_holdAimDownSight && weapInput.secondary)) {
				if (player->IsToolWeapon()) {
					// there is a possibility that player has respawned or something.
					// stop aiming down
					weapInput.secondary = false;
				}
			}

			// is the selected tool no longer usable (ex. out of ammo)?
			if (!player->IsToolSelectable(player->GetTool())) {
				// release mouse button before auto-switching tools
				winp.primary = false;
				winp.secondary = false;
				weapInput = winp;
				net->SendWeaponInput(weapInput);
				actualWeapInput = winp = player->GetWeaponInput();

				// select another tool
				Player::ToolType t = player->GetTool();
				do {
					switch (t) {
						case Player::ToolSpade: t = Player::ToolGrenade; break;
						case Player::ToolBlock: t = Player::ToolSpade; break;
						case Player::ToolWeapon: t = Player::ToolBlock; break;
						case Player::ToolGrenade: t = Player::ToolWeapon; break;
					}
				} while (!world->GetLocalPlayer()->IsToolSelectable(t));
				SetSelectedTool(t);
			}

			// send orientation
			Vector3 curFront = player->GetFront();
			if (curFront.x != lastFront.x || curFront.y != lastFront.y ||
			    curFront.z != lastFront.z) {
				lastFront = curFront;
				net->SendOrientation(curFront);
			}

			lastKills = world->GetPlayerPersistent(player->GetId()).kills;

			// show block count when building block lines.
			if (player->IsAlive() && player->GetTool() == Player::ToolBlock &&
			    player->GetWeaponInput().secondary && player->IsBlockCursorDragging()) {
				if (player->IsBlockCursorActive()) {
					auto blocks = world->CubeLine(player->GetBlockCursorDragPos(),
												  player->GetBlockCursorPos(), 256);
					auto msg = _TrN("Client", "{0} block", "{0} blocks", blocks.size());
					AlertType type = static_cast<int>(blocks.size()) > player->GetNumBlocks()
					                   ? AlertType::Warning
					                   : AlertType::Notice;
					ShowAlert(msg, type, 0.f, true);
				} else {
					// invalid
					auto msg = _Tr("Client", "-- blocks");
					AlertType type = AlertType::Warning;
					ShowAlert(msg, type, 0.f, true);
				}
			}

			if (player->IsAlive())
				lastAliveTime = time;

			if (player->GetHealth() < lastHealth) {
				// ouch!
				lastHealth = player->GetHealth();
				lastHurtTime = world->GetTime();

				Handle<IAudioChunk> c;
				switch (SampleRandomInt(0, 3)) {
					case 0:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal1.opus");
						break;
					case 1:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal2.opus");
						break;
					case 2:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal3.opus");
						break;
					case 3:
						c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/FleshLocal4.opus");
						break;
				}
				audioDevice->PlayLocal(c, AudioParam());

				float hpper = player->GetHealth() / 100.f;
				int cnt = 18 - (int)(player->GetHealth() / 100.f * 8.f);
				hurtSprites.resize(std::max(cnt, 6));
				for (size_t i = 0; i < hurtSprites.size(); i++) {
					HurtSprite &spr = hurtSprites[i];
					spr.angle = SampleRandomFloat() * (2.f * static_cast<float>(M_PI));
					spr.scale = .2f + SampleRandomFloat() * SampleRandomFloat() * .7f;
					spr.horzShift = SampleRandomFloat();
					spr.strength = .3f + SampleRandomFloat() * .7f;
					if (hpper > .5f) {
						spr.strength *= 1.5f - hpper;
					}
				}

			} else {
				lastHealth = player->GetHealth();
			}

			inp.jump = false;
		}

#pragma mark - IWorldListener Handlers

		void Client::PlayerObjectSet(int id) {
			if (clientPlayers[id]) {
				clientPlayers[id]->Invalidate();
				clientPlayers[id] = nullptr;
			}

			Player *p = world->GetPlayer(id);
			if (p)
				clientPlayers[id].Set(new ClientPlayer(p, this), false);
		}

		void Client::PlayerJumped(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {

				Handle<IAudioChunk> c =
				  p->GetWade() ? audioDevice->RegisterSound("Sounds/Player/WaterJump.opus")
				               : audioDevice->RegisterSound("Sounds/Player/Jump.opus");
				audioDevice->Play(c, p->GetOrigin(), AudioParam());
			}
		}

		void Client::PlayerLanded(spades::client::Player *p, bool hurt) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				Handle<IAudioChunk> c;
				if (hurt)
					c = audioDevice->RegisterSound("Sounds/Player/FallHurt.opus");
				else if (p->GetWade())
					c = audioDevice->RegisterSound("Sounds/Player/WaterLand.opus");
				else
					c = audioDevice->RegisterSound("Sounds/Player/Land.opus");
				audioDevice->Play(c, p->GetOrigin(), AudioParam());
			}
		}

		void Client::PlayerMadeFootstep(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
                std::array<const char *, 8> snds = {"Sounds/Player/Footstep1.opus", "Sounds/Player/Footstep2.opus",
				                      "Sounds/Player/Footstep3.opus", "Sounds/Player/Footstep4.opus",
				                      "Sounds/Player/Footstep5.opus", "Sounds/Player/Footstep6.opus",
				                      "Sounds/Player/Footstep7.opus", "Sounds/Player/Footstep8.opus"};
				std::array<const char *, 12> rsnds = {
				  "Sounds/Player/Run1.opus",  "Sounds/Player/Run2.opus",  "Sounds/Player/Run3.opus",
				  "Sounds/Player/Run4.opus",  "Sounds/Player/Run5.opus",  "Sounds/Player/Run6.opus",
				  "Sounds/Player/Run7.opus",  "Sounds/Player/Run8.opus",  "Sounds/Player/Run9.opus",
				  "Sounds/Player/Run10.opus", "Sounds/Player/Run11.opus", "Sounds/Player/Run12.opus",
				};
				std::array<const char *, 8> wsnds = {"Sounds/Player/Wade1.opus", "Sounds/Player/Wade2.opus",
				                       "Sounds/Player/Wade3.opus", "Sounds/Player/Wade4.opus",
				                       "Sounds/Player/Wade5.opus", "Sounds/Player/Wade6.opus",
				                       "Sounds/Player/Wade7.opus", "Sounds/Player/Wade8.opus"};
				bool sprinting = clientPlayers[p->GetId()]
				                   ? clientPlayers[p->GetId()]->GetSprintState() > 0.5f
				                   : false;
				Handle<IAudioChunk> c =
				  p->GetWade() ? audioDevice->RegisterSound(SampleRandomElement(wsnds))
				               : audioDevice->RegisterSound(SampleRandomElement(snds));
				audioDevice->Play(c, p->GetOrigin(), AudioParam());
				if (sprinting && !p->GetWade()) {
					AudioParam param;
					param.volume *= clientPlayers[p->GetId()]->GetSprintState();
					c = audioDevice->RegisterSound(SampleRandomElement(rsnds));
					audioDevice->Play(c, p->GetOrigin(), param);
				}
			}
		}

		void Client::PlayerFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (p == world->GetLocalPlayer()) {
				localFireVibrationTime = time;
			}

			clientPlayers[p->GetId()]->FiredWeapon();
		}
		void Client::PlayerDryFiredWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/DryFire.opus");
				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f - p->GetUp() * .3f +
					                       p->GetRight() * .4f,
					                  AudioParam());
			}
		}

		void Client::PlayerReloadingWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			clientPlayers[p->GetId()]->ReloadingWeapon();
		}

		void Client::PlayerReloadedWeapon(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			clientPlayers[p->GetId()]->ReloadedWeapon();
		}

		void Client::PlayerChangedTool(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c;
				if (isLocal) {
					// played by ClientPlayer::Update
					return;
				} else {
					c = audioDevice->RegisterSound("Sounds/Weapons/Switch.opus");
				}
				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f - p->GetUp() * .3f +
					                       p->GetRight() * .4f,
					                  AudioParam());
			}
		}

		void Client::PlayerRestocked(spades::client::Player *p) {
			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c =
				  isLocal ? audioDevice->RegisterSound("Sounds/Weapons/RestockLocal.opus")
				          : audioDevice->RegisterSound("Sounds/Weapons/Restock.opus");
				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f - p->GetUp() * .3f +
					                       p->GetRight() * .4f,
					                  AudioParam());
			}
		}

		void Client::PlayerThrownGrenade(spades::client::Player *p, Grenade *g) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c =
				  audioDevice->RegisterSound("Sounds/Weapons/Grenade/Throw.opus");

				if (g && isLocal) {
					net->SendGrenade(g);
				}

				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.4f, 0.1f, .3f), AudioParam());
				else
					audioDevice->Play(c, p->GetEye() + p->GetFront() * 0.5f - p->GetUp() * .2f +
					                       p->GetRight() * .3f,
					                  AudioParam());
			}
		}

		void Client::PlayerMissedSpade(spades::client::Player *p) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Spade/Miss.opus");
				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.2f, -.1f, 0.7f), AudioParam());
				else
					audioDevice->Play(c, p->GetOrigin() + p->GetFront() * 0.8f - p->GetUp() * .2f,
					                  AudioParam());
			}
		}

		void Client::PlayerHitBlockWithSpade(spades::client::Player *p, Vector3 hitPos,
		                                     IntVector3 blockPos, IntVector3 normal) {
			SPADES_MARK_FUNCTION();

			uint32_t col = map->GetColor(blockPos.x, blockPos.y, blockPos.z);
			IntVector3 colV = {(uint8_t)col, (uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			Vector3 shiftedHitPos = hitPos;
			shiftedHitPos.x += normal.x * .05f;
			shiftedHitPos.y += normal.y * .05f;
			shiftedHitPos.z += normal.z * .05f;

			EmitBlockFragments(shiftedHitPos, colV);

			if (p == world->GetLocalPlayer()) {
				localFireVibrationTime = time;
			}

			if (!IsMuted()) {
				bool isLocal = p == world->GetLocalPlayer();
				Handle<IAudioChunk> c =
				  audioDevice->RegisterSound("Sounds/Weapons/Spade/HitBlock.opus");
				if (isLocal)
					audioDevice->PlayLocal(c, MakeVector3(.1f, -.1f, 1.2f), AudioParam());
				else
					audioDevice->Play(c, p->GetOrigin() + p->GetFront() * 0.5f - p->GetUp() * .2f,
					                  AudioParam());
			}
		}

		void Client::PlayerKilledPlayer(spades::client::Player *killer,
		                                spades::client::Player *victim, KillType kt) {
			// play hit sound
			if (kt == KillTypeWeapon || kt == KillTypeHeadshot) {
				// don't play on local: see BullethitPlayer
				if (victim != world->GetLocalPlayer()) {
					if (!IsMuted()) {
						Handle<IAudioChunk> c;
						switch (SampleRandomInt(0, 2)) {
							case 0:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh1.opus");
								break;
							case 1:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh2.opus");
								break;
							case 2:
								c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh3.opus");
								break;
						}
						AudioParam param;
						param.volume = 4.f;
						audioDevice->Play(c, victim->GetEye(), param);
					}
				}
			}

			// The local player is dead; initialize the look-you-are-dead cam
			if (victim == world->GetLocalPlayer()) {
				followCameraState.enabled = false;

				Vector3 v = -victim->GetFront();
				followAndFreeCameraState.yaw = atan2(v.y, v.x);
				followAndFreeCameraState.pitch = 30.f * M_PI / 180.f;
			}

			// emit blood (also for local player)
			// FIXME: emiting blood for either
			// client-side or server-side hit?
			switch (kt) {
				case KillTypeGrenade:
				case KillTypeHeadshot:
				case KillTypeMelee:
				case KillTypeWeapon: Bleed(victim->GetEye()); break;
				default: break;
			}

			// create ragdoll corpse
			if (cg_ragdoll && victim->GetTeamId() < 2) {
				Corpse *corp;
				corp = new Corpse(renderer, map, victim);
				if (victim == world->GetLocalPlayer())
					lastMyCorpse = corp;
				if (killer != victim && kt != KillTypeGrenade) {
					Vector3 dir = victim->GetPosition() - killer->GetPosition();
					dir = dir.Normalize();
					if (kt == KillTypeMelee) {
						dir *= 6.f;
					} else {
						if (killer->GetWeapon()->GetWeaponType() == SMG_WEAPON) {
							dir *= 2.8f;
						} else if (killer->GetWeapon()->GetWeaponType() == SHOTGUN_WEAPON) {
							dir *= 4.5f;
						} else {
							dir *= 3.5f;
						}
					}
					corp->AddImpulse(dir);
				} else if (kt == KillTypeGrenade) {
					corp->AddImpulse(MakeVector3(0, 0, -4.f - SampleRandomFloat() * 4.f));
				}
				corp->AddImpulse(victim->GetVelocity() * 32.f);
				corpses.emplace_back(corp);

				if (corpses.size() > corpseHardLimit) {
					corpses.pop_front();
				} else if (corpses.size() > corpseSoftLimit) {
					RemoveInvisibleCorpses();
				}
			}

			// add chat message
			std::string s;
			s = ChatWindow::TeamColorMessage(killer->GetName(), killer->GetTeamId());

			std::string cause;
			bool isFriendlyFire = killer->GetTeamId() == victim->GetTeamId();
			if (killer == victim)
				isFriendlyFire = false;

			Weapon *w =
			  killer ? killer->GetWeapon() : nullptr; // only used in case of KillTypeWeapon
			switch (kt) {
				case KillTypeWeapon:
					switch (w ? w->GetWeaponType() : RIFLE_WEAPON) {
						case RIFLE_WEAPON: cause += _Tr("Client", "Rifle"); break;
						case SMG_WEAPON: cause += _Tr("Client", "SMG"); break;
						case SHOTGUN_WEAPON: cause += _Tr("Client", "Shotgun"); break;
					}
					break;
				case KillTypeFall:
					//! A cause of death shown in the kill feed.
					cause += _Tr("Client", "Fall");
					break;
				case KillTypeMelee:
					//! A cause of death shown in the kill feed.
					cause += _Tr("Client", "Melee");
					break;
				case KillTypeGrenade:
					cause += _Tr("Client", "Grenade");
					break;
				case KillTypeHeadshot:
					//! A cause of death shown in the kill feed.
					cause += _Tr("Client", "Headshot");
					break;
				case KillTypeTeamChange:
					//! A cause of death shown in the kill feed.
					cause += _Tr("Client", "Team Change");
					break;
				case KillTypeClassChange:
					//! A cause of death shown in the kill feed.
					cause += _Tr("Client", "Weapon Change");
					break;
				default:
					cause += "???";
					break;
			}

			s += " [";
			if (isFriendlyFire)
				s += ChatWindow::ColoredMessage(cause, MsgColorFriendlyFire);
			else if (killer == world->GetLocalPlayer() || victim == world->GetLocalPlayer())
				s += ChatWindow::ColoredMessage(cause, MsgColorGray);
			else
				s += cause;
			s += "] ";

			if (killer != victim) {
				s += ChatWindow::TeamColorMessage(victim->GetName(), victim->GetTeamId());
			}

			killfeedWindow->AddMessage(s);

			// log to netlog
			if (killer != victim) {
				NetLog("%s (%s) [%s] %s (%s)", killer->GetName().c_str(),
				       world->GetTeam(killer->GetTeamId()).name.c_str(), cause.c_str(),
				       victim->GetName().c_str(), world->GetTeam(victim->GetTeamId()).name.c_str());
			} else {
				NetLog("%s (%s) [%s]", killer->GetName().c_str(),
				       world->GetTeam(killer->GetTeamId()).name.c_str(), cause.c_str());
			}

			// show big message if player is involved
			if (victim != killer) {
				Player *local = world->GetLocalPlayer();
				if (killer == local || victim == local) {
					std::string msg;
					if (killer == local) {
						if ((int)cg_centerMessage == 2)
							msg = _Tr("Client", "You have killed {0}", victim->GetName());
					} else {
						msg = _Tr("Client", "You were killed by {0}", killer->GetName());
					}
					centerMessageView->AddMessage(msg);
				}
			}
		}

		void Client::BulletHitPlayer(spades::client::Player *hurtPlayer, HitType type,
		                             spades::Vector3 hitPos, spades::client::Player *by) {
			SPADES_MARK_FUNCTION();

			SPAssert(type != HitTypeBlock);

			// don't bleed local player
			if (!IsFirstPerson(GetCameraMode()) || &GetCameraTargetPlayer() != hurtPlayer) {
				Bleed(hitPos);
			}

			if (hurtPlayer == world->GetLocalPlayer()) {
				// don't player hit sound now;
				// local bullet impact sound is
				// played by checking the decrease of HP
				return;
			}

			if (!IsMuted()) {
				if (type == HitTypeMelee) {
					Handle<IAudioChunk> c =
					  audioDevice->RegisterSound("Sounds/Weapons/Spade/HitPlayer.opus");
					audioDevice->Play(c, hitPos, AudioParam());
				} else {
					Handle<IAudioChunk> c;
					switch (SampleRandomInt(0, 2)) {
						case 0:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh1.opus");
							break;
						case 1:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh2.opus");
							break;
						case 2:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Flesh3.opus");
							break;
					}
					AudioParam param;
					param.volume = 4.f;
					audioDevice->Play(c, hitPos, param);
				}
			}

			if (by == world->GetLocalPlayer() && hurtPlayer) {
				net->SendHit(hurtPlayer->GetId(), type);

				if (type == HitTypeHead) {
					Handle<IAudioChunk> c =
					  audioDevice->RegisterSound("Sounds/Feedback/HeadshotFeedback.opus");
					AudioParam param;
					param.volume = cg_hitFeedbackSoundGain;
					audioDevice->PlayLocal(c, param);
				}

				hitFeedbackIconState = 1.f;
				if (hurtPlayer->GetTeamId() == world->GetLocalPlayer()->GetTeamId()) {
					hitFeedbackFriendly = true;
				} else {
					hitFeedbackFriendly = false;
				}
			}
		}

		void Client::BulletHitBlock(Vector3 hitPos, IntVector3 blockPos, IntVector3 normal) {
			SPADES_MARK_FUNCTION();

			uint32_t col = map->GetColor(blockPos.x, blockPos.y, blockPos.z);
			IntVector3 colV = {(uint8_t)col, (uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			Vector3 shiftedHitPos = hitPos;
			shiftedHitPos.x += normal.x * .05f;
			shiftedHitPos.y += normal.y * .05f;
			shiftedHitPos.z += normal.z * .05f;

			if (blockPos.z == 63) {
				BulletHitWaterSurface(shiftedHitPos);
				if (!IsMuted()) {
					AudioParam param;
					param.volume = 2.f;

					Handle<IAudioChunk> c;

					param.pitch = .9f + SampleRandomFloat() * 0.2f;
					switch (SampleRandomInt(0, 3)) {
						case 0:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Water1.opus");
							break;
						case 1:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Water2.opus");
							break;
						case 2:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Water3.opus");
							break;
						case 3:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Water4.opus");
							break;
					}
					audioDevice->Play(c, shiftedHitPos, param);
				}
			} else {
				EmitBlockFragments(shiftedHitPos, colV);

				if (!IsMuted()) {
					AudioParam param;
					param.volume = 2.f;

					Handle<IAudioChunk> c;
                    c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Block.opus");
					audioDevice->Play(c, shiftedHitPos, param);

					param.pitch = .9f + SampleRandomFloat() * 0.2f;
					param.volume = 2.f;
					switch (SampleRandomInt(0, 3)) {
						case 0:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet1.opus");
							break;
						case 1:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet2.opus");
							break;
						case 2:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet3.opus");
							break;
						case 3:
							c = audioDevice->RegisterSound("Sounds/Weapons/Impacts/Ricochet4.opus");
							break;
					}
					audioDevice->Play(c, shiftedHitPos, param);
				}
			}
		}

		void Client::AddBulletTracer(spades::client::Player *player, spades::Vector3 muzzlePos,
		                             spades::Vector3 hitPos) {
			SPADES_MARK_FUNCTION();

			// Do not display tracers for bullets fired by the local player
			if (IsFirstPerson(GetCameraMode()) && GetCameraTargetPlayerId() == player->GetId()) {
				return;
			}

			float vel;
			switch (player->GetWeapon()->GetWeaponType()) {
				case RIFLE_WEAPON: vel = 700.f; break;
				case SMG_WEAPON: vel = 360.f; break;
				case SHOTGUN_WEAPON: vel = 500.f; break;
			}
			AddLocalEntity(new Tracer(this, muzzlePos, hitPos, vel));
			AddLocalEntity(new MapViewTracer(muzzlePos, hitPos, vel));
		}

		void Client::BlocksFell(std::vector<IntVector3> blocks) {
			SPADES_MARK_FUNCTION();

			if (blocks.empty())
				return;
			FallingBlock *b = new FallingBlock(this, blocks);
			AddLocalEntity(b);

			if (!IsMuted()) {

				IntVector3 v = blocks[0];
				Vector3 o;
				o.x = v.x;
				o.y = v.y;
				o.z = v.z;
				o += .5f;

				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Misc/BlockFall.opus");
				audioDevice->Play(c, o, AudioParam());
			}
		}

		void Client::GrenadeBounced(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();

			if (g->GetPosition().z < 63.f) {
				if (!IsMuted()) {
					Handle<IAudioChunk> c =
					  audioDevice->RegisterSound("Sounds/Weapons/Grenade/Bounce.opus");
					audioDevice->Play(c, g->GetPosition(), AudioParam());
				}
			}
		}

		void Client::GrenadeDroppedIntoWater(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				Handle<IAudioChunk> c =
				  audioDevice->RegisterSound("Sounds/Weapons/Grenade/DropWater.opus");
				audioDevice->Play(c, g->GetPosition(), AudioParam());
			}
		}

		void Client::GrenadeExploded(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();

			bool inWater = g->GetPosition().z > 63.f;

			if (inWater) {
				if (!IsMuted()) {
					Handle<IAudioChunk> c =
					  audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplode.opus");
					AudioParam param;
					param.volume = 10.f;
					audioDevice->Play(c, g->GetPosition(), param);

					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplodeFar.opus");
					param.volume = 6.f;
					param.referenceDistance = 10.f;
					audioDevice->Play(c, g->GetPosition(), param);

					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/WaterExplodeStereo.opus");
					param.volume = 2.f;
					audioDevice->Play(c, g->GetPosition(), param);
				}

				GrenadeExplosionUnderwater(g->GetPosition());
			} else {

				GrenadeExplosion(g->GetPosition());

				if (!IsMuted()) {
					Handle<IAudioChunk> c, cs;

					switch (SampleRandomInt(0, 1)) {
						case 0:
							c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode1.opus");
							cs = audioDevice->RegisterSound(
							  "Sounds/Weapons/Grenade/ExplodeStereo1.opus");
							break;
						case 1:
							c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Explode2.opus");
							cs = audioDevice->RegisterSound(
							  "Sounds/Weapons/Grenade/ExplodeStereo2.opus");
							break;
					}

					AudioParam param;
					param.volume = 30.f;
					param.referenceDistance = 5.f;
					audioDevice->Play(c, g->GetPosition(), param);

					param.referenceDistance = 1.f;
					audioDevice->Play(cs, g->GetPosition(), param);

					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeFar.opus");
					param.volume = 6.f;
					param.referenceDistance = 40.f;
					audioDevice->Play(c, g->GetPosition(), param);

					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/ExplodeFarStereo.opus");
					param.referenceDistance = 10.f;
					audioDevice->Play(c, g->GetPosition(), param);

					// debri sound
					c = audioDevice->RegisterSound("Sounds/Weapons/Grenade/Debris.opus");
					param.volume = 5.f;
					param.referenceDistance = 3.f;
					IntVector3 outPos;
					Vector3 soundPos = g->GetPosition();
					if (world->GetMap()->CastRay(soundPos, MakeVector3(0, 0, 1), 8.f, outPos)) {
						soundPos.z = (float)outPos.z - .2f;
					}
					audioDevice->Play(c, soundPos, param);
				}
			}
		}

		void Client::LocalPlayerPulledGrenadePin() {
			SPADES_MARK_FUNCTION();

			if (!IsMuted()) {
				Handle<IAudioChunk> c =
				  audioDevice->RegisterSound("Sounds/Weapons/Grenade/Fire.opus");
				audioDevice->PlayLocal(c, MakeVector3(.4f, -.3f, .5f), AudioParam());
			}
		}

		void Client::LocalPlayerBlockAction(spades::IntVector3 v, BlockActionType type) {
			SPADES_MARK_FUNCTION();
			net->SendBlockAction(v, type);
		}
		void Client::LocalPlayerCreatedLineBlock(spades::IntVector3 v1, spades::IntVector3 v2) {
			SPADES_MARK_FUNCTION();
			net->SendBlockLine(v1, v2);
		}

		void Client::LocalPlayerHurt(HurtType type, bool sourceGiven, spades::Vector3 source) {
			SPADES_MARK_FUNCTION();

			if (sourceGiven) {
				Player *p = world->GetLocalPlayer();
				if (!p)
					return;
				Vector3 rel = source - p->GetEye();
				rel.z = 0.f;
				rel = rel.Normalize();
				hurtRingView->Add(rel);
			}
		}

		void Client::LocalPlayerBuildError(BuildFailureReason reason) {
			SPADES_MARK_FUNCTION();

			if (!cg_alerts) {
				PlayAlertSound();
				return;
			}

			switch (reason) {
				case BuildFailureReason::InsufficientBlocks:
					ShowAlert(_Tr("Client", "Insufficient blocks."), AlertType::Error);
					break;
				case BuildFailureReason::InvalidPosition:
					ShowAlert(_Tr("Client", "You cannot place a block there."), AlertType::Error);
					break;
			}
		}
	}
}
