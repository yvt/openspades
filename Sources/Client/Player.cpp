/*
 Copyright (c) 2013 yvt,
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

#include "Player.h"

#include "GameMap.h"
#include "GameMapWrapper.h"
#include "Grenade.h"
#include "HitTestDebugger.h"
#include "IWorldListener.h"
#include "PhysicsConstants.h"
#include "Weapon.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>

namespace spades {
	namespace client {

		Player::Player(World *w, int playerId, WeaponType wType, int teamId, Vector3 position,
		               IntVector3 color)
		    : world(w) {
			SPADES_MARK_FUNCTION();

			lastClimbTime = -100;
			lastJumpTime = -100;
			lastJump = false;
			tool = ToolWeapon;
			airborne = false;
			wade = false;
			this->position = position;
			velocity = MakeVector3(0, 0, 0);
			orientation = MakeVector3(1, 0, 0);
			if (teamId) // quick hack for correct spawn orientation
				orientation = MakeVector3(-1, 0, 0);
			eye = MakeVector3(0, 0, 0);
			moveDistance = 0.f;
			moveSteps = 0;

			this->playerId = playerId;
			this->weapon = Weapon::CreateWeapon(wType, this, *w->GetGameProperties());
			this->weaponType = wType;
			this->teamId = teamId;
			this->weapon->Reset();
			this->color = color;

			health = 100;
			grenades = 3;
			blockStocks = 50;
			blockColor = IntVector3::Make(111, 111, 111);

			nextSpadeTime = 0.f;
			nextDigTime = 0.f;
			nextGrenadeTime = 0.f;
			nextBlockTime = 0.f;
			firstDig = false;
			lastReloadingTime = 0.f;

			pendingPlaceBlock = false;
			pendingRestockBlock = false;

			blockCursorActive = false;
			blockCursorDragging = false;

			holdingGrenade = false;
			reloadingServerSide = false;
			canPending = false;
		}

		Player::~Player() {
			SPADES_MARK_FUNCTION();
			delete weapon;
		}

		bool Player::IsLocalPlayer() {
			if (!world)
				return false;
			return world->GetLocalPlayer() == this;
		}

		void Player::SetInput(PlayerInput newInput) {
			SPADES_MARK_FUNCTION();

			if (!IsAlive())
				return;

			if (newInput.crouch != input.crouch && !airborne) {
				if (newInput.crouch)
					position.z += 0.9f;
				else
					position.z -= 0.9f;
			}
			input = newInput;
		}

		void Player::SetWeaponInput(WeaponInput newInput) {
			SPADES_MARK_FUNCTION();
			auto *listener = GetWorld()->GetListener();

			if (!IsAlive())
				return;

			if (input.sprint && !input.crouch &&
			    (input.moveBackward || input.moveForward || input.moveLeft || input.moveRight)) {
				newInput.primary = false;
				newInput.secondary = false;
			}
			if (tool == ToolSpade) {
				if (newInput.secondary)
					newInput.primary = false;
				if (newInput.secondary != weapInput.secondary) {
					if (newInput.secondary) {
						nextDigTime = world->GetTime() + 1.f;
						firstDig = true;
					}
				}
			} else if (tool == ToolGrenade) {
				if (world->GetTime() < nextGrenadeTime) {
					newInput.primary = false;
				}
				if (grenades == 0) {
					newInput.primary = false;
				}
				if (weapInput.primary && holdingGrenade && GetGrenadeCookTime() < .15f) {
					// pin is not pulled yet
					newInput.primary = true;
				}
				if (newInput.primary != weapInput.primary) {
					if (!newInput.primary) {
						if (holdingGrenade) {
							nextGrenadeTime = world->GetTime() + .5f;
							ThrowGrenade();
						}
					} else {
						holdingGrenade = true;
						grenadeTime = world->GetTime();
						if (listener && this == world->GetLocalPlayer())
							// playing other's grenade sound
							// is cheating
							listener->LocalPlayerPulledGrenadePin();
					}
				}
			} else if (tool == ToolBlock) {
				// work-around for bug that placing block
				// occasionally becomes impossible
				if (nextBlockTime >
				    world->GetTime() + std::max(GetToolPrimaryDelay(), GetToolSecondaryDelay())) {
					nextBlockTime =
					  world->GetTime() + std::max(GetToolPrimaryDelay(), GetToolSecondaryDelay());
				}

				if (world->GetTime() < nextBlockTime) {
					newInput.primary = false;
					newInput.secondary = false;
				}
				if (newInput.secondary)
					newInput.primary = false;
				if (newInput.secondary != weapInput.secondary) {
					if (newInput.secondary) {
						if (IsBlockCursorActive()) {
							blockCursorDragging = true;
							blockCursorDragPos = blockCursorPos;
						} else {
							// cannot build; invalid position.
							if (listener && this == world->GetLocalPlayer()) {
								listener->LocalPlayerBuildError(
								  BuildFailureReason::InvalidPosition);
							}
						}
					} else {
						if (IsBlockCursorDragging()) {
							if (IsBlockCursorActive()) {
								std::vector<IntVector3> blocks =
								  GetWorld()->CubeLine(blockCursorDragPos, blockCursorPos, 256);
								if ((int)blocks.size() <= blockStocks) {
									if (listener && this == world->GetLocalPlayer())
										listener->LocalPlayerCreatedLineBlock(blockCursorDragPos,
										                                      blockCursorPos);
									// blockStocks -= blocks.size(); decrease when created
								} else {
									// cannot build; insufficient blocks.
									if (listener && this == world->GetLocalPlayer()) {
										listener->LocalPlayerBuildError(
										  BuildFailureReason::InsufficientBlocks);
									}
								}
								nextBlockTime = world->GetTime() + GetToolSecondaryDelay();
							} else {
								// cannot build; invalid position.
								if (listener && this == world->GetLocalPlayer()) {
									listener->LocalPlayerBuildError(
									  BuildFailureReason::InvalidPosition);
								}
							}
						}

						blockCursorDragging = false;
						blockCursorActive = false;
					}
				}
				if (newInput.primary != weapInput.primary || newInput.primary) {
					if (newInput.primary) {

						if (!weapInput.primary)
							lastSingleBlockBuildSeqDone = false;
						if (IsBlockCursorActive() && blockStocks > 0) {
							if (listener && this == world->GetLocalPlayer())
								listener->LocalPlayerBlockAction(blockCursorPos, BlockActionCreate);

							lastSingleBlockBuildSeqDone = true;
							// blockStocks--; decrease when created

							nextBlockTime = world->GetTime() + GetToolPrimaryDelay();
						} else if (blockStocks > 0 && airborne && canPending &&
						           this == world->GetLocalPlayer()) {
							pendingPlaceBlock = true;
							pendingPlaceBlockPos = blockCursorPos;
						} else if (!IsBlockCursorActive()) {
							// wait for building becoming possible
						}

						blockCursorDragging = false;
						blockCursorActive = false;
					} else {
						if (!lastSingleBlockBuildSeqDone) {
							// cannot build; invalid position.
							if (listener && this == world->GetLocalPlayer()) {
								listener->LocalPlayerBuildError(
								  BuildFailureReason::InvalidPosition);
							}
						}
					}
				}
			} else if (IsToolWeapon()) {
				weapon->SetShooting(newInput.primary);

				// Update the weapon state asap so it picks up the weapon fire event even
				// if the player presses the mouse button and releases it really fast.
				// We shouldn't do this for the local player because the client haven't sent
				// a weapon update packet at this point and the hit will be rejected by the server.
				if (!IsLocalPlayer() && weapon->FrameNext(0.0f)) {
					FireWeapon();
				}
			} else {
				SPAssert(false);
			}

			weapInput = newInput;
		}

		void Player::Reload() {
			SPADES_MARK_FUNCTION();
			if (health == 0) {
				// dead man cannot reload
				return;
			}
			weapon->Reload();
			if (this == world->GetLocalPlayer() && weapon->IsReloading())
				reloadingServerSide = true;
		}

		void Player::ReloadDone(int clip, int stock) {
			reloadingServerSide = false;
			weapon->ReloadDone(clip, stock);
		}

		void Player::Restock() {
			SPADES_MARK_FUNCTION();
			if (health == 0) {
				// dead man cannot restock
				return;
			}

			weapon->Restock();
			grenades = 3;
			pendingRestockBlock = true;
			health = 100;

			if (world->GetListener())
				world->GetListener()->PlayerRestocked(this);
		}

		void Player::GotBlock() {
			if (blockStocks < 50)
				blockStocks++;
		}

		void Player::SetTool(spades::client::Player::ToolType t) {
			SPADES_MARK_FUNCTION();

			if (t == tool)
				return;
			tool = t;
			holdingGrenade = false;
			blockCursorActive = false;
			blockCursorDragging = false;

			reloadingServerSide = false;

			WeaponInput inp;
			SetWeaponInput(inp);

			weapon->AbortReload();

			if (world->GetListener())
				world->GetListener()->PlayerChangedTool(this);
		}

		void Player::SetHeldBlockColor(spades::IntVector3 col) { blockColor = col; }

		void Player::SetPosition(const spades::Vector3 &v) {
			SPADES_MARK_FUNCTION();

			position = v;
			eye = v;
		}

		void Player::SetVelocity(const spades::Vector3 &v) {
			SPADES_MARK_FUNCTION();

			velocity = v;
		}

		void Player::SetOrientation(const spades::Vector3 &v) {
			SPADES_MARK_FUNCTION();

			orientation = v;
		}

		void Player::Turn(float longitude, float latitude) {
			SPADES_MARK_FUNCTION();

			Vector3 o = GetFront();
			float lng = atan2f(o.y, o.x);
			float lat = atan2f(o.z, sqrtf(o.x * o.x + o.y * o.y));

			lng += longitude;
			lat += latitude;

			if (lat < -static_cast<float>(M_PI) * .49f)
				lat = -static_cast<float>(M_PI) * .49f;
			if (lat > static_cast<float>(M_PI) * .49f)
				lat = static_cast<float>(M_PI) * .49f;

			o.x = cosf(lng) * cosf(lat);
			o.y = sinf(lng) * cosf(lat);
			o.z = sinf(lat);
			SetOrientation(o);
		}

		void Player::SetHP(int hp, HurtType type, spades::Vector3 p) {
			health = hp;
			if (this == world->GetLocalPlayer()) {
				if (world->GetListener())
					world->GetListener()->LocalPlayerHurt(
					  type, p.x != 0.f || p.y != 0.f || p.z != 0.f, p);
			}
		}

		void Player::Update(float dt) {
			SPADES_MARK_FUNCTION();
			auto *listener = world->GetListener();

			MovePlayer(dt);

			if (!IsAlive()) {
				// do death cleanup
				blockCursorDragging = false;
			}

			if (tool == ToolSpade) {
				if (weapInput.primary) {
					if (world->GetTime() > nextSpadeTime) {
						UseSpade();
						nextSpadeTime = world->GetTime() + GetToolPrimaryDelay();
					}
				} else if (weapInput.secondary) {
					if (world->GetTime() > nextDigTime) {
						DigWithSpade();
						nextDigTime = world->GetTime() + GetToolSecondaryDelay();
						firstDig = false;
					}
				}
			} else if (tool == ToolBlock && IsLocalPlayer()) {
				GameMap::RayCastResult result;
				auto *map = GetWorld()->GetMap();
				result = map->CastRay2(GetEye(), GetFront(), 12);
				canPending = false;

				if (blockCursorDragging) {
					// check the starting point is not floating
					auto start = blockCursorDragPos;
					if (map->IsSolidWrapped(start.x - 1, start.y, start.z) ||
					    map->IsSolidWrapped(start.x, start.y - 1, start.z) ||
					    map->IsSolidWrapped(start.x, start.y, start.z - 1) ||
					    map->IsSolidWrapped(start.x + 1, start.y, start.z) ||
					    map->IsSolidWrapped(start.x, start.y + 1, start.z) ||
					    map->IsSolidWrapped(start.x, start.y, start.z + 1)) {
						// still okay
					} else {
						// cannot build; floating
						if (listener && this == world->GetLocalPlayer()) {
							listener->LocalPlayerBuildError(BuildFailureReason::InvalidPosition);
						}
						blockCursorDragging = false;
					}
				}

				if (result.hit && (result.hitBlock + result.normal).z < 62 &&
				    (!OverlapsWithOneBlock(result.hitBlock + result.normal)) &&
				    BoxDistanceToBlock(result.hitBlock + result.normal) < 3.f &&
				    (result.hitBlock + result.normal).z >= 0 &&
				    !pendingPlaceBlock) {

					// Building is possible, and there's no delayed block placement.
					blockCursorActive = true;
					blockCursorPos = result.hitBlock + result.normal;

				} else if (pendingPlaceBlock) {

					// Delayed Block Placement: When player attempts to place a block while jumping
					// and
					// placing block is currently impossible, building will be delayed until it
					// becomes
					// possible, as long as player is airborne.
					if (airborne == false || blockStocks <= 0) {
						// player is no longer airborne, or doesn't have a block to place.
						pendingPlaceBlock = false;
						lastSingleBlockBuildSeqDone = true;
						if (blockStocks > 0) {
							// cannot build; invalid position.
						}
					} else if ((!OverlapsWithOneBlock(pendingPlaceBlockPos)) &&
					           BoxDistanceToBlock(pendingPlaceBlockPos) < 3.f) {
						// now building became possible.
						SPAssert(this == world->GetLocalPlayer());

						if (GetWorld()->GetListener())
							GetWorld()->GetListener()->LocalPlayerBlockAction(pendingPlaceBlockPos,
							                                                  BlockActionCreate);

						pendingPlaceBlock = false;
						lastSingleBlockBuildSeqDone = true;
						// blockStocks--; decrease when created

						nextBlockTime = world->GetTime() + GetToolPrimaryDelay();
					}

				} else {
					// Delayed Block Placement can be activated only when the only reason making
					// placement
					// impossible is that block to be placed overlaps with the player's hitbox.
					canPending = result.hit && (result.hitBlock + result.normal).z < 62 &&
					             BoxDistanceToBlock(result.hitBlock + result.normal) < 3.f;

					blockCursorActive = false;
					int dist = 11;
					for (; dist >= 1 && BoxDistanceToBlock(result.hitBlock + result.normal) > 3.f;
					     dist--) {
						result = GetWorld()->GetMap()->CastRay2(GetEye(), GetFront(), dist);
					}
					for (; dist < 12 && BoxDistanceToBlock(result.hitBlock + result.normal) < 3.f;
					     dist++) {
						result = GetWorld()->GetMap()->CastRay2(GetEye(), GetFront(), dist);
					}

					blockCursorPos = result.hitBlock + result.normal;
				}

			} else if (tool == ToolWeapon) {
			} else if (tool == ToolGrenade) {
				if (holdingGrenade) {
					if (world->GetTime() - grenadeTime > 2.9f) {
						ThrowGrenade();
					}
				}
			}

			if (tool != ToolWeapon)
				weapon->SetShooting(false);
			if (weapon->FrameNext(dt)) {
				FireWeapon();
			}

			if (weapon->IsReloading()) {
				lastReloadingTime = world->GetTime();
			} else if (reloadingServerSide) {
				// for some reason a server didn't return
				// WeaponReload packet.
				if (world->GetTime() + lastReloadingTime + .8f) {
					reloadingServerSide = false;
					weapon->ForceReloadDone();
				}
			}

			if (pendingRestockBlock) {
				blockStocks = 50;
				pendingRestockBlock = false;
			}
		}

		bool Player::RayCastApprox(spades::Vector3 start, spades::Vector3 dir) {
			Vector3 diff = position - start;

			// |P-A| * cos(theta)
			float c = Vector3::Dot(diff, dir);

			// |P-A|^2
			float sq = diff.GetPoweredLength();

			// |P-A| * sin(theta)
			float dist = sqrtf(sq - c * c);

			return dist < 8.f;
		}

		static float GetHorizontalLength(const Vector3 &v) {
			return std::sqrt(v.x * v.x + v.y * v.y);
		}

		enum class HitBodyPart { None, Head, Torso, Limb1, Limb2, Arms };

		void Player::FireWeapon() {
			SPADES_MARK_FUNCTION();

			Vector3 muzzle = GetEye();
			muzzle += GetFront() * 0.01f;

			// for hit-test debugging
			std::map<int, HitTestDebugger::PlayerHit> playerHits;
			std::vector<Vector3> bulletVectors;

			// Vector3 right = GetRight();
			// Vector3 up = GetUp();

			int pellets = weapon->GetPelletSize();
			float spread = weapon->GetSpread();
			GameMap *map = world->GetMap();

			if (!weapInput.secondary) {
				spread *= 2;
			}

			// pyspades takes destroying more than one block as a
			// speed hack (shotgun does this)
			bool blockDestroyed = false;

			Vector3 dir2 = GetFront();
			for (int i = 0; i < pellets; i++) {

				// AoS 0.75's way (dir2 shouldn't be normalized!)
				dir2.x += (SampleRandomFloat() - SampleRandomFloat()) * spread;
				dir2.y += (SampleRandomFloat() - SampleRandomFloat()) * spread;
				dir2.z += (SampleRandomFloat() - SampleRandomFloat()) * spread;
				Vector3 dir = dir2.Normalize();

				bulletVectors.push_back(dir);

				// first do map raycast
				GameMap::RayCastResult mapResult;
				mapResult = map->CastRay2(muzzle, dir, 500);

				Player *hitPlayer = NULL;
				float hitPlayerDistance = 0.f; // disregarding Z coordinate
				float hitPlayerActualDistance = 0.f;
				HitBodyPart hitPart = HitBodyPart::None;

				for (int i = 0; i < world->GetNumPlayerSlots(); i++) {
					Player *other = world->GetPlayer(i);
					if (other == this || other == NULL)
						continue;
					if (other == this || !other->IsAlive() || other->GetTeamId() >= 2)
						continue;
					// quickly reject players unlikely to be hit
					if (!other->RayCastApprox(muzzle, dir))
						continue;

					HitBoxes hb = other->GetHitBoxes();
					Vector3 hitPos;

					if (hb.head.RayCast(muzzle, dir, &hitPos)) {
						float dist = GetHorizontalLength(hitPos - muzzle);
						if (hitPlayer == NULL || dist < hitPlayerDistance) {
							hitPlayer = other;
							hitPlayerDistance = dist;
							hitPlayerActualDistance = (hitPos - muzzle).GetLength();
							hitPart = HitBodyPart::Head;
						}
					}
					if (hb.torso.RayCast(muzzle, dir, &hitPos)) {
						float dist = GetHorizontalLength(hitPos - muzzle);
						if (hitPlayer == NULL || dist < hitPlayerDistance) {
							hitPlayer = other;
							hitPlayerDistance = dist;
							hitPlayerActualDistance = (hitPos - muzzle).GetLength();
							hitPart = HitBodyPart::Torso;
						}
					}
					for (int j = 0; j < 3; j++) {
						if (hb.limbs[j].RayCast(muzzle, dir, &hitPos)) {
							float dist = GetHorizontalLength(hitPos - muzzle);
							if (hitPlayer == NULL || dist < hitPlayerDistance) {
								hitPlayer = other;
								hitPlayerDistance = dist;
								hitPlayerActualDistance = (hitPos - muzzle).GetLength();
								switch (j) {
									case 0: hitPart = HitBodyPart::Limb1; break;
									case 1: hitPart = HitBodyPart::Limb2; break;
									case 2: hitPart = HitBodyPart::Arms; break;
								}
							}
						}
					}
				}

				Vector3 finalHitPos;
				finalHitPos = muzzle + dir * 128.f;

				if (hitPlayer == nullptr && !mapResult.hit) {
					// might hit water surface.
				}

				if (mapResult.hit && GetHorizontalLength(mapResult.hitPos - muzzle) < 128.f &&
				    (hitPlayer == NULL ||
				     GetHorizontalLength(mapResult.hitPos - muzzle) < hitPlayerDistance)) {
					IntVector3 outBlockCoord = mapResult.hitBlock;
					// TODO: set correct ray distance
					// FIXME: why ray casting twice?

					finalHitPos = mapResult.hitPos;

					if (outBlockCoord.x >= 0 && outBlockCoord.y >= 0 && outBlockCoord.z >= 0 &&
					    outBlockCoord.x < map->Width() && outBlockCoord.y < map->Height() &&
					    outBlockCoord.z < map->Depth()) {
						if (outBlockCoord.z == 63) {
							if (world->GetListener())
								world->GetListener()->BulletHitBlock(
								  mapResult.hitPos, mapResult.hitBlock, mapResult.normal);
						} else if (outBlockCoord.z == 62) {
							// blocks at this level cannot be damaged
							if (world->GetListener())
								world->GetListener()->BulletHitBlock(
								  mapResult.hitPos, mapResult.hitBlock, mapResult.normal);
						} else {
							int x = outBlockCoord.x;
							int y = outBlockCoord.y;
							int z = outBlockCoord.z;
							SPAssert(map->IsSolid(x, y, z));

							Vector3 blockF = {x + .5f, y + .5f, z + .5f};
							float distance = GetHorizontalLength(blockF - muzzle);

							uint32_t color = map->GetColor(x, y, z);
							int health = color >> 24;
							health -= weapon->GetDamage(HitTypeBlock, distance);
							if (health <= 0 && !blockDestroyed) {
								health = 0;
								blockDestroyed = true;
								// send destroy cmd
								if (world->GetListener() && world->GetLocalPlayer() == this)
									world->GetListener()->LocalPlayerBlockAction(outBlockCoord,
									                                             BlockActionTool);
							}
							color = (color & 0xffffff) | ((uint32_t)health << 24);
							if (map->IsSolid(x, y, z))
								map->Set(x, y, z, true, color);

							world->MarkBlockForRegeneration(outBlockCoord);

							if (world->GetListener())
								world->GetListener()->BulletHitBlock(
								  mapResult.hitPos, mapResult.hitBlock, mapResult.normal);
						}
					}
				} else if (hitPlayer != NULL) {
					if (hitPlayerDistance < 128.f) {

						finalHitPos = muzzle + dir * hitPlayerActualDistance;

						switch (hitPart) {
							case HitBodyPart::Head:
								playerHits[hitPlayer->GetId()].numHeadHits++;
								break;
							case HitBodyPart::Torso:
								playerHits[hitPlayer->GetId()].numTorsoHits++;
								break;
							case HitBodyPart::Limb1:
								playerHits[hitPlayer->GetId()].numLimbHits[0]++;
								break;
							case HitBodyPart::Limb2:
								playerHits[hitPlayer->GetId()].numLimbHits[1]++;
								break;
							case HitBodyPart::Arms:
								playerHits[hitPlayer->GetId()].numLimbHits[2]++;
								break;
							case HitBodyPart::None: SPAssert(false); break;
						}

						if (world->GetListener()) {
							switch (hitPart) {
								case HitBodyPart::Head:
									world->GetListener()->BulletHitPlayer(
									  hitPlayer, HitTypeHead, finalHitPos,
									  this);
									break;
								case HitBodyPart::Torso:
									world->GetListener()->BulletHitPlayer(
									  hitPlayer, HitTypeTorso, finalHitPos,
									  this);
									break;
								case HitBodyPart::Limb1:
								case HitBodyPart::Limb2:
									world->GetListener()->BulletHitPlayer(
									  hitPlayer, HitTypeLegs, finalHitPos,
									  this);
									break;
								case HitBodyPart::Arms:
									world->GetListener()->BulletHitPlayer(
									  hitPlayer, HitTypeArms, finalHitPos,
									  this);
									break;
								case HitBodyPart::None: SPAssert(false); break;
							}
						}
					}
				}

				if (world->GetListener() && this != world->GetLocalPlayer())
					world->GetListener()->AddBulletTracer(this, muzzle, finalHitPos);

				// one pellet done
			}

			// do hit test debugging
			auto *debugger = world->GetHitTestDebugger();
			if (debugger && IsLocalPlayer()) {
				debugger->SaveImage(playerHits, bulletVectors);
			}

			// in AoS 0.75's way
			Vector3 o = orientation;
			Vector3 rec = weapon->GetRecoil();
			float upLimit = Vector3::Dot(GetFront2D(), o);
			upLimit -= 0.03f; // ???

			// vanilla's horizontial recoil is driven by a triangular wave generator.
			int time = (int) (world->GetTime() * 1000);
			float triWave;
			if (time % 1024 < 512) {
				triWave = (time % 512) - 255.5;
			} else {
				triWave = 255.5 - (time % 512);
			}

			float horzModifier = 1;
			float vertModifier = 1;

			if ((input.moveLeft || input.moveRight || input.moveForward || input.moveBackward) && !weapInput.secondary) {
				horzModifier *= 2;
				vertModifier *= 2;
			}

			if (airborne) {
				horzModifier *= 2;
				vertModifier *= 2;
			}
			else if (input.crouch) {
				horzModifier /= 2;
				vertModifier /= 2;
			}

			horzModifier *= sqrt(1 - pow(o.z, 4));

			o += GetRight() * rec.x * triWave * horzModifier;
			o += GetUp() * std::min(rec.y, std::max(0.f, upLimit)) * vertModifier;
			o = o.Normalize();
			SetOrientation(o);

			reloadingServerSide = false;
		}

		void Player::ThrowGrenade() {
			SPADES_MARK_FUNCTION();

			if (!holdingGrenade)
				return;
			grenades--;

			Vector3 muzzle = GetEye() + GetFront() * 0.1f;
			Vector3 vel = GetFront() * 1.f;
			float fuse = world->GetTime() - grenadeTime;
			fuse = 3.f - fuse;

			if (health <= 0) {
				// drop, don't throw
				vel = MakeVector3(0, 0, 0);
			}

			vel += GetVelocity();

			if (this == world->GetLocalPlayer()) {
				Grenade *gren = new Grenade(world, muzzle, vel, fuse);
				world->AddGrenade(gren);
				if (world->GetListener())
					world->GetListener()->PlayerThrownGrenade(this, gren);
			} else {
				// grenade packet will be sent by server
				if (world->GetListener())
					world->GetListener()->PlayerThrownGrenade(this, NULL);
			}

			holdingGrenade = false;
		}

		void Player::DigWithSpade() {
			SPADES_MARK_FUNCTION();

			IntVector3 outBlockCoord;
			GameMap *map = world->GetMap();
			Vector3 muzzle = GetEye(), dir = GetFront();

			// TODO: set correct ray distance
			// first do map raycast
			GameMap::RayCastResult mapResult;
			mapResult = map->CastRay2(muzzle, dir, 256);

			outBlockCoord = mapResult.hitBlock;

			// TODO: set correct ray distance
			if (mapResult.hit && BoxDistanceToBlock(mapResult.hitBlock + mapResult.normal) < 3.f &&
			    outBlockCoord.x >= 0 && outBlockCoord.y >= 0 && outBlockCoord.z >= 0 &&
			    outBlockCoord.x < map->Width() && outBlockCoord.y < map->Height() &&
			    outBlockCoord.z < map->Depth()) {
				if (outBlockCoord.z < 62) {
					SPAssert(map->IsSolid(outBlockCoord.x, outBlockCoord.y, outBlockCoord.z));

					// send destroy command only for local cmd
					if (this == world->GetLocalPlayer()) {

						if (world->GetListener())
							world->GetListener()->LocalPlayerBlockAction(outBlockCoord,
							                                             BlockActionDig);
					}

					if (world->GetListener())
						world->GetListener()->PlayerHitBlockWithSpade(
						  this, mapResult.hitPos, mapResult.hitBlock, mapResult.normal);
				}
			} else {
				if (world->GetListener())
					world->GetListener()->PlayerMissedSpade(this);
			}
		}

		void Player::UseSpade() {
			SPADES_MARK_FUNCTION();

			bool missed = true;

			Vector3 muzzle = GetEye(), dir = GetFront();

			IntVector3 outBlockCoord;
			GameMap *map = world->GetMap();
			// TODO: set correct ray distance
			// first do map raycast
			GameMap::RayCastResult mapResult;
			mapResult = map->CastRay2(muzzle, dir, 256);

			Player *hitPlayer = NULL;
			int hitFlag = 0;

			for (int i = 0; i < world->GetNumPlayerSlots(); i++) {
				Player *other = world->GetPlayer(i);
				if (other == this || other == NULL)
					continue;
				if (other == this || !other->IsAlive() || other->GetTeamId() >= 2)
					continue;
				if (!other->RayCastApprox(muzzle, dir))
					continue;
				if ((eye - other->GetEye()).GetChebyshevLength() >= MELEE_DISTANCE_F)
					continue;

				Vector3 diff = other->GetEye() - eye;
				Vector3 view;
				view.x = Vector3::Dot(diff, GetRight());
				view.y = Vector3::Dot(diff, GetUp());
				view.z = Vector3::Dot(diff, GetFront());

				if (view.z < 0.f)
					continue;

				view.x /= view.z;
				view.y /= view.z;
				view.z = 0.f;

				if (view.GetChebyshevLength() < 5.f) {
					hitPlayer = other;
					hitFlag = 1;
					break;
				}
			}

			outBlockCoord = mapResult.hitBlock;
			if (mapResult.hit && BoxDistanceToBlock(mapResult.hitBlock + mapResult.normal) < 3.f &&
			    (hitPlayer == NULL) && outBlockCoord.x >= 0 && outBlockCoord.y >= 0 &&
			    outBlockCoord.z >= 0 && outBlockCoord.x < map->Width() &&
			    outBlockCoord.y < map->Height() && outBlockCoord.z < map->Depth()) {
				if (outBlockCoord.z < 62) {
					int x = outBlockCoord.x;
					int y = outBlockCoord.y;
					int z = outBlockCoord.z;
					SPAssert(map->IsSolid(x, y, z));
					missed = false;

					uint32_t color = map->GetColor(x, y, z);
					int health = color >> 24;
					health -= 55;
					if (health <= 0) {
						health = 0;
						// send destroy command only for local cmd
						if (this == world->GetLocalPlayer()) {
							if (world->GetListener())
								world->GetListener()->LocalPlayerBlockAction(outBlockCoord,
								                                             BlockActionTool);
						}
					}
					color = (color & 0xffffff) | ((uint32_t)health << 24);
					if (map->IsSolid(x, y, z))
						map->Set(x, y, z, true, color);

					world->MarkBlockForRegeneration(outBlockCoord);

					if (world->GetListener())
						world->GetListener()->PlayerHitBlockWithSpade(
						  this, mapResult.hitPos, mapResult.hitBlock, mapResult.normal);
				}
			} else if (hitPlayer != NULL) {

				if (world->GetListener()) {
					if (hitFlag)
						world->GetListener()->BulletHitPlayer(hitPlayer, HitTypeMelee,
						                                      hitPlayer->GetEye(), this);
				}
			}

			if (missed) {
				if (world->GetListener())
					world->GetListener()->PlayerMissedSpade(this);
			}
		}

		Vector3 Player::GetFront() {
			SPADES_MARK_FUNCTION_DEBUG();
			return orientation;
		}

		Vector3 Player::GetFront2D() {
			SPADES_MARK_FUNCTION_DEBUG();
			return MakeVector3(orientation.x, orientation.y, 0.f).Normalize();
		}

		Vector3 Player::GetRight() {
			SPADES_MARK_FUNCTION_DEBUG();
			return -Vector3::Cross(MakeVector3(0, 0, -1), GetFront2D()).Normalize();
		}

		Vector3 Player::GetLeft() {
			SPADES_MARK_FUNCTION_DEBUG();
			return -GetRight();
		}

		Vector3 Player::GetUp() {
			SPADES_MARK_FUNCTION_DEBUG();
			return Vector3::Cross(GetRight(), GetFront()).Normalize();
		}

		bool Player::GetWade() {
			SPADES_MARK_FUNCTION_DEBUG();
			return GetOrigin().z > 62.f;
		}

		Vector3 Player::GetOrigin() {
			SPADES_MARK_FUNCTION_DEBUG();
			Vector3 v = eye;
			v.z += (input.crouch ? .45f : .9f);
			v.z += .3f;
			return v;
		}

		void Player::BoxClipMove(float fsynctics) {
			SPADES_MARK_FUNCTION();

			float f = fsynctics * 32.f;
			float nx = f * velocity.x + position.x;
			float ny = f * velocity.y + position.y;
			bool climb = false;
			float offset, m;
			if (input.crouch) {
				offset = .45f;
				m = .9f;
			} else {
				offset = .9f;
				m = 1.35f;
			}

			float nz = position.z + offset;

			float z;
			GameMap *map = world->GetMap();

			if (velocity.x < 0.f)
				f = -0.45f;
			else
				f = 0.45f;

			z = m;

			while (z >= -1.36f && !map->ClipBox(nx + f, position.y - .45f, nz + z) &&
			       !map->ClipBox(nx + f, position.y + .45f, nz + z))
				z -= 0.9f;
			if (z < -1.36f)
				position.x = nx;
			else if (!(input.crouch) && orientation.z < 0.5f && !input.sprint) {
				z = 0.35f;
				while (z >= -2.36f && !map->ClipBox(nx + f, position.y - .45f, nz + z) &&
				       !map->ClipBox(nx + f, position.y + .45f, nz + z))
					z -= 0.9f;
				if (z < -2.36f) {
					position.x = nx;
					climb = true;
				} else {
					velocity.x = 0.f;
				}
			} else {
				velocity.x = 0.f;
			}

			if (velocity.y < 0.f)
				f = -0.45f;
			else
				f = 0.45f;

			z = m;

			while (z >= -1.36f && !map->ClipBox(position.x - .45f, ny + f, nz + z) &&
			       !map->ClipBox(position.x + .45f, ny + f, nz + z))
				z -= 0.9f;
			if (z < -1.36f)
				position.y = ny;
			else if (!(input.crouch) && orientation.z < 0.5f && !input.sprint && !climb) {
				z = 0.35f;
				while (z >= -2.36f && !map->ClipBox(position.x - .45f, ny + f, nz + z) &&
				       !map->ClipBox(position.x + .45f, ny + f, nz + z))
					z -= 0.9f;
				if (z < -2.36f) {
					position.y = ny;
					climb = true;
				} else {
					velocity.y = 0.f;
				}
			} else if (!climb) {
				velocity.y = 0.f;
			}

			if (climb) {
				velocity.x *= .5f;
				velocity.y *= .5f;
				lastClimbTime = world->GetTime();
				nz -= 1.f;
				m = -1.35f;
			} else {
				if (velocity.z < 0.f)
					m = -m;
				nz += velocity.z * fsynctics * 32.f;
			}

			airborne = true;
			if (map->ClipBox(position.x - .45f, position.y - .45f, nz + m) ||
			    map->ClipBox(position.x - .45f, position.y + .45f, nz + m) ||
			    map->ClipBox(position.x + .45f, position.y - .45f, nz + m) ||
			    map->ClipBox(position.x + .45f, position.y + .45f, nz + m)) {
				if (velocity.z >= 0.f) {
					wade = position.z > 61.f;
					airborne = false;
				}
				velocity.z = 0.f;
			} else {
				position.z = nz - offset;
			}

			RepositionPlayer(position);
		}

		bool Player::IsOnGroundOrWade() {
			return ((velocity.z >= 0.f && velocity.z < .017f) && !airborne);
		}

		void Player::ForceJump() {
			velocity.z = -0.36f;
			lastJump = true;
			if (world->GetListener() && world->GetTime() > lastJumpTime + .1f) {
				world->GetListener()->PlayerJumped(this);
				lastJumpTime = world->GetTime();
			}
		}

		void Player::MovePlayer(float fsynctics) {
			if (input.jump && (!lastJump) && IsOnGroundOrWade()) {
				velocity.z = -0.36f;
				lastJump = true;
				if (world->GetListener() && world->GetTime() > lastJumpTime + .1f) {
					world->GetListener()->PlayerJumped(this);
					lastJumpTime = world->GetTime();
				}
			} else if (!input.jump) {
				lastJump = false;
			}

			float f = fsynctics;
			if (airborne)
				f *= 0.1f;
			else if (input.crouch)
				f *= 0.3f;
			else if ((weapInput.secondary && IsToolWeapon()) || input.sneak)
				f *= 0.5f;
			else if (input.sprint)
				f *= 1.3f;
			if ((input.moveForward || input.moveBackward) && (input.moveRight || input.moveLeft))
				f /= sqrtf(2.f);

			// looking up or down should alter speed
			const float maxVertLookSlowdown = 0.9f;
			const float vertLookSlowdownStart = 0.65f; // about 40 degrees
			float slowdownByVertLook =
			  std::max(std::abs(GetFront().z) - vertLookSlowdownStart, 0.0f) /
			  (1.0f - vertLookSlowdownStart) * maxVertLookSlowdown;

			Vector3 front = GetFront2D() * (1.0f - slowdownByVertLook);
			Vector3 left = GetLeft();

			if (input.moveForward) {
				velocity.x += front.x * f;
				velocity.y += front.y * f;
			} else if (input.moveBackward) {
				velocity.x -= front.x * f;
				velocity.y -= front.y * f;
			}
			if (input.moveLeft) {
				velocity.x += left.x * f;
				velocity.y += left.y * f;
			} else if (input.moveRight) {
				velocity.x -= left.x * f;
				velocity.y -= left.y * f;
			}

			// this is a linear approximation that's
			// done in pysnip
			// accurate computation is not difficult
			f = fsynctics + 1.f;
			velocity.z += fsynctics;
			velocity.z /= f; // air friction

			if (wade)
				f = fsynctics * 6.f + 1.f;
			else if (!airborne)
				f = fsynctics * 4.f + 1.f;

			velocity.x /= f;
			velocity.y /= f;

			float f2 = velocity.z;
			BoxClipMove(fsynctics);

			// hit ground
			if (velocity.z == 0.f && (f2 > FALL_SLOW_DOWN)) {
				velocity.x *= .5f;
				velocity.y *= .5f;

				if (f2 > FALL_DAMAGE_VELOCITY) {
					if (world->GetListener()) {
						world->GetListener()->PlayerLanded(this, true);
					}
				} else {
					if (world->GetListener()) {
						world->GetListener()->PlayerLanded(this, false);
					}
				}
			}

			if (velocity.z >= 0.f && velocity.z < .017f && !input.sneak && !input.crouch &&
			    !(weapInput.secondary && IsToolWeapon())) {
				// count move distance
				f = fsynctics * 32.f;
				float dx = f * velocity.x;
				float dy = f * velocity.y;
				float dist = sqrtf(dx * dx + dy * dy);
				moveDistance += dist * .3f;

				bool madeFootstep = false;
				while (moveDistance > 1.f) {
					moveSteps++;
					moveDistance -= 1.f;

					if (world->GetListener() && !madeFootstep) {
						world->GetListener()->PlayerMadeFootstep(this);
						madeFootstep = true;
					}
				}
			}
		}

		bool Player::TryUncrouch(bool move) {
			SPADES_MARK_FUNCTION();

			float x1 = position.x + 0.45f;
			float x2 = position.x - 0.45f;
			float y1 = position.y + 0.45f;
			float y2 = position.y - 0.45f;
			float z1 = position.z + 2.25f;
			float z2 = position.z - 1.35f;

			GameMap *map = world->GetMap();

			// lower feet
			if (airborne &&
			    !(map->ClipBox(x1, y1, z1) || map->ClipBox(x2, y1, z1) ||
			      map->ClipBox(x1, y2, z1) || map->ClipBox(x2, y2, z1)))
				return true;
			else if (!(map->ClipBox(x1, y1, z2) || map->ClipBox(x2, y1, z2) ||
			           map->ClipBox(x1, y2, z2) || map->ClipBox(x2, y2, z2))) {
				if (move) {
					position.z -= 0.9f;
					eye.z -= 0.9f;
				}
				return true;
			}
			return false;
		}

		void Player::RepositionPlayer(const spades::Vector3 &pos2) {
			SPADES_MARK_FUNCTION();

			eye = position = pos2;
			float f = lastClimbTime - world->GetTime();
			if (f > -.25f)
				eye.z += (f + .25f) / .25f;
		}

		float Player::GetToolPrimaryDelay() {
			SPADES_MARK_FUNCTION_DEBUG();

			switch (tool) {
				case ToolWeapon: return weapon->GetDelay();
				case ToolBlock: return .5f;
				case ToolSpade: return .2f;
				case ToolGrenade: return .5f;
				default: SPInvalidEnum("tool", tool);
			}
		}

		float Player::GetToolSecondaryDelay() {
			SPADES_MARK_FUNCTION_DEBUG();

			switch (tool) {
				case ToolBlock: return GetToolPrimaryDelay();
				case ToolSpade: return 1.f;
				default: SPInvalidEnum("tool", tool);
			}
		}

		float Player::GetSpadeAnimationProgress() {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(tool == ToolSpade);
			SPAssert(weapInput.primary);
			return 1.f - (nextSpadeTime - world->GetTime()) / GetToolPrimaryDelay();
		}

		float Player::GetDigAnimationProgress() {
			SPADES_MARK_FUNCTION_DEBUG();

			SPAssert(tool == ToolSpade);
			SPAssert(weapInput.secondary);
			return 1.f - (nextDigTime - world->GetTime()) / GetToolSecondaryDelay();
		}

		float Player::GetTimeToNextGrenade() { return nextGrenadeTime - world->GetTime(); }

		void Player::KilledBy(KillType type, Player *killer, int respawnTime) {
			SPADES_MARK_FUNCTION();
			health = 0;
			weapon->SetShooting(false);

			// if local player is killed while cooking grenade,
			// drop the live grenade.
			if (this == world->GetLocalPlayer() && tool == ToolGrenade && holdingGrenade) {
				ThrowGrenade();
			}
			if (world->GetListener())
				world->GetListener()->PlayerKilledPlayer(killer, this, type);

			input = PlayerInput();
			weapInput = WeaponInput();
			this->respawnTime = world->GetTime() + respawnTime;
		}

		bool Player::IsAlive() { return health > 0; }

		std::string Player::GetName() { return world->GetPlayerPersistent(GetId()).name; }

		float Player::GetWalkAnimationProgress() {
			return moveDistance * .5f + (float)(moveSteps)*.5f;
		}

		Player::HitBoxes Player::GetHitBoxes() {
			SPADES_MARK_FUNCTION_DEBUG();
			Player::HitBoxes hb;

			Vector3 front = GetFront();

			float yaw = atan2(front.y, front.x) + static_cast<float>(M_PI) * .5f;
			float pitch = -atan2(front.z, sqrt(front.x * front.x + front.y * front.y));

			// lower axis
			Matrix4 lower = Matrix4::Translate(GetOrigin());
			lower = lower * Matrix4::Rotate(MakeVector3(0, 0, 1), yaw);

			Matrix4 torso;

			if (input.crouch) {
				lower = lower * Matrix4::Translate(0, 0, -0.4f);
				// lower
				hb.limbs[0] = AABB3(-.4f, -.15f, 0.5f, 0.3f, .3f, 0.5f);
				hb.limbs[0] = lower * hb.limbs[0];

				hb.limbs[1] = AABB3(.1f, -.15f, 0.5f, 0.3f, .3f, 0.5f);
				hb.limbs[1] = lower * hb.limbs[1];

				torso = lower * Matrix4::Translate(0, 0, -0.3f);

				// torso
				hb.torso = AABB3(-.4f, -.15f, 0.1f, .8f, .8f, .6f);
				hb.torso = torso * hb.torso;

				hb.limbs[2] = AABB3(-.6f, -.15f, 0.1f, 1.2f, .3f, .6f);
				hb.limbs[2] = torso * hb.limbs[2];

				// head
				hb.head = AABB3(-.3f, -.3f, -0.45f, .6f, .6f, 0.6f);
				hb.head = Matrix4::Translate(0, 0, -0.15f) * hb.head;
				hb.head = Matrix4::Rotate(MakeVector3(1, 0, 0), pitch) * hb.head;
				hb.head = Matrix4::Translate(0, 0, 0.15f) * hb.head;
				hb.head = torso * hb.head;
			} else {
				// lower
				hb.limbs[0] = AABB3(-.4f, -.15f, 0.f, 0.3f, .3f, 1.f);
				hb.limbs[0] = lower * hb.limbs[0];

				hb.limbs[1] = AABB3(.1f, -.15f, 0.f, 0.3f, .3f, 1.f);
				hb.limbs[1] = lower * hb.limbs[1];

				torso = lower * Matrix4::Translate(0, 0, -1.1f);

				// torso
				hb.torso = AABB3(-.4f, -.15f, 0.1f, .8f, .3f, .9f);
				hb.torso = torso * hb.torso;

				hb.limbs[2] = AABB3(-.6f, -.15f, 0.1f, 1.2f, .3f, .9f);
				hb.limbs[2] = torso * hb.limbs[2];

				// head
				hb.head = AABB3(-.3f, -.3f, -0.5f, .6f, .6f, 0.6f);
				hb.head = Matrix4::Translate(0, 0, -0.1f) * hb.head;
				hb.head = Matrix4::Rotate(MakeVector3(1, 0, 0), pitch) * hb.head;
				hb.head = Matrix4::Translate(0, 0, 0.1f) * hb.head;
				hb.head = torso * hb.head;
			}

			return hb;
		}
		IntVector3 Player::GetColor() { return world->GetTeam(teamId).color; }

		bool Player::IsCookingGrenade() { return tool == ToolGrenade && holdingGrenade; }
		float Player::GetGrenadeCookTime() { return world->GetTime() - grenadeTime; }

		void Player::SetWeaponType(WeaponType weap) {
			SPADES_MARK_FUNCTION_DEBUG();
			if (this->weapon->GetWeaponType() == weap)
				return;
			delete this->weapon;
			this->weapon = Weapon::CreateWeapon(weap, this, *world->GetGameProperties());
			this->weaponType = weap;
		}

		void Player::SetTeam(int tId) { teamId = tId; }

		bool Player::IsReadyToUseTool() {
			SPADES_MARK_FUNCTION_DEBUG();
			switch (tool) {
				case ToolBlock: return world->GetTime() > nextBlockTime && blockStocks > 0;
				case ToolGrenade: return world->GetTime() > nextGrenadeTime && grenades > 0;
				case ToolSpade: return true;
				case ToolWeapon: return weapon->IsReadyToShoot();
			}
		}

		bool Player::IsToolSelectable(ToolType type) {
			SPADES_MARK_FUNCTION_DEBUG();
			switch (type) {
				case ToolSpade: return true;
				case ToolBlock: return blockStocks > 0;
				case ToolWeapon: return weapon->GetAmmo() > 0 || weapon->GetStock() > 0;
				case ToolGrenade: return grenades > 0;
				default: SPAssert(false);
			}
		}

		bool Player::OverlapsWith(const spades::AABB3 &aabb) {
			SPADES_MARK_FUNCTION_DEBUG();
			float offset, m;
			if (input.crouch) {
				offset = .45f;
				m = .9f;
			} else {
				offset = .9f;
				m = 1.35f;
			}
			m -= .5f;
			AABB3 playerBox(eye.x - .45f, eye.y - .45f, eye.z, .9f, .9f, offset + m);
			return aabb && playerBox;
		}

		bool Player::OverlapsWithOneBlock(spades::IntVector3 vec) {
			SPADES_MARK_FUNCTION_DEBUG();
			return OverlapsWith(AABB3(vec.x, vec.y, vec.z, 1, 1, 1));
		}

#pragma mark - Block Construction
		bool Player::IsBlockCursorActive() { return tool == ToolBlock && blockCursorActive; }
		bool Player::IsBlockCursorDragging() { return tool == ToolBlock && blockCursorDragging; }
		float Player::BoxDistanceToBlock(spades::IntVector3 v) {
			Vector3 e = {(float)v.x, (float)v.y, (float)v.z};
			e += .5f;

			return (e - eye).GetChebyshevLength();
		}
	}
}
