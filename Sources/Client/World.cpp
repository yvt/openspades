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
#include <deque>

#include "GameMap.h"
#include "GameMapWrapper.h"
#include "GameProperties.h"
#include "Grenade.h"
#include "HitTestDebugger.h"
#include "IGameMode.h"
#include "IWorldListener.h"
#include "Player.h"
#include "Weapon.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Settings.h>

DEFINE_SPADES_SETTING(cg_debugHitTest, "0");

namespace spades {
	namespace client {

		World::World(const std::shared_ptr<GameProperties> &gameProperties)
		    : gameProperties{gameProperties} {
			SPADES_MARK_FUNCTION();
		}
		World::~World() { SPADES_MARK_FUNCTION(); }

		size_t World::GetNumPlayers() {
			size_t numPlayers = 0;
			for (const auto &p : players) {
				if (p)
					++numPlayers;
			}
			return numPlayers;
		}

		void World::Advance(float dt) {
			SPADES_MARK_FUNCTION();

			ApplyBlockActions();

			for (const auto &player : players)
				if (player)
					player->Update(dt);

			while (!blockRegenerationQueue.empty()) {
				auto it = blockRegenerationQueue.begin();
				if (it->first > time) {
					break;
				}

				const IntVector3 &block = it->second;

				if (map && map->IsSolid(block.x, block.y, block.z)) {
					uint32_t color = map->GetColor(block.x, block.y, block.z);
					uint32_t health = 100;
					color = (color & 0xffffff) | (health << 24);
					map->Set(block.x, block.y, block.z, true, color);
				}

				blockRegenerationQueueMap.erase(blockRegenerationQueueMap.find(it->second));
				blockRegenerationQueue.erase(it);
			}

			std::vector<decltype(grenades.begin())> removedGrenades;
			for (auto it = grenades.begin(); it != grenades.end(); it++) {
				Grenade &g = **it;
				if (g.Update(dt)) {
					removedGrenades.push_back(it);
				}
			}
			for (auto it : removedGrenades)
				grenades.erase(it);

			time += dt;
		}

		void World::SetMap(Handle<GameMap> newMap) {
			if (map == newMap)
				return;

			hitTestDebugger.reset();

			if (map) {
				mapWrapper.reset();
			}

			map = newMap;
			if (map) {
				mapWrapper = stmp::make_unique<GameMapWrapper>(*map);
				mapWrapper->Rebuild();
			}
		}

		void World::AddGrenade(std::unique_ptr<Grenade> g) {
			SPADES_MARK_FUNCTION_DEBUG();

			grenades.push_back(std::move(g));
		}

		void World::SetPlayer(int i, std::unique_ptr<Player> p) {
			SPADES_MARK_FUNCTION();

			players.at(i) = std::move(p);
			if (listener) {
				listener->PlayerObjectSet(i);
			}
		}

		void World::SetMode(std::unique_ptr<IGameMode> m) { mode = std::move(m); }

		void World::MarkBlockForRegeneration(const IntVector3 &blockLocation) {
			UnmarkBlockForRegeneration(blockLocation);

			// Regenerate after 10 seconds
			auto result = blockRegenerationQueue.emplace(time + 10.0f, blockLocation);
			blockRegenerationQueueMap.emplace(blockLocation, result);
		}

		void World::UnmarkBlockForRegeneration(const IntVector3 &blockLocation) {
			auto it = blockRegenerationQueueMap.find(blockLocation);
			if (it == blockRegenerationQueueMap.end()) {
				return;
			}

			blockRegenerationQueue.erase(it->second);
			blockRegenerationQueueMap.erase(it);
		}

		static std::vector<std::vector<CellPos>>
		ClusterizeBlocks(const std::vector<CellPos> &blocks) {
			std::unordered_map<CellPos, bool, CellPosHash> blockMap;
			for (const auto &block : blocks) {
				blockMap[block] = true;
			}

			std::vector<std::vector<CellPos>> ret;
			std::deque<decltype(blockMap)::iterator> queue;

			ret.reserve(64);
			// wish I could `reserve()` queue...

			std::size_t addedCount = 0;

			for (auto it = blockMap.begin(); it != blockMap.end(); it++) {
				SPAssert(queue.empty());

				if (!it->second)
					continue;
				queue.emplace_back(it);

				std::vector<CellPos> outBlocks;

				while (!queue.empty()) {
					auto blockitem = queue.front();
					queue.pop_front();

					if (!blockitem->second)
						continue;

					auto pos = blockitem->first;
					outBlocks.emplace_back(pos);
					blockitem->second = false;

					decltype(blockMap)::iterator nextIt;

					nextIt = blockMap.find(CellPos(pos.x - 1, pos.y, pos.z));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
					nextIt = blockMap.find(CellPos(pos.x + 1, pos.y, pos.z));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
					nextIt = blockMap.find(CellPos(pos.x, pos.y - 1, pos.z));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
					nextIt = blockMap.find(CellPos(pos.x, pos.y + 1, pos.z));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
					nextIt = blockMap.find(CellPos(pos.x, pos.y, pos.z - 1));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
					nextIt = blockMap.find(CellPos(pos.x, pos.y, pos.z + 1));
					if (nextIt != blockMap.end() && nextIt->second) {
						queue.emplace_back(nextIt);
					}
				}

				SPAssert(!outBlocks.empty());
				addedCount += outBlocks.size();
				ret.emplace_back(std::move(outBlocks));
			}

			SPAssert(addedCount == blocks.size());

			return ret;
		}

		void World::ApplyBlockActions() {
			for (const auto &creation : createdBlocks) {
				const auto &pos = creation.first;
				const auto &color = creation.second;
				if (map->IsSolid(pos.x, pos.y, pos.z)) {
					map->Set(pos.x, pos.y, pos.z, true,
					         color.x | (color.y << 8) | (color.z << 16) | (100UL << 24));
					continue;
				}
				mapWrapper->AddBlock(pos.x, pos.y, pos.z,
				                     color.x | (color.y << 8) | (color.z << 16) | (100UL << 24));
			}

			std::vector<CellPos> cells;
			for (const auto &cell : destroyedBlocks) {
				if (!map->IsSolid(cell.x, cell.y, cell.z))
					continue;
				cells.emplace_back(cell);
			}

			cells = mapWrapper->RemoveBlocks(cells);

			auto clusters = ClusterizeBlocks(cells);
			std::vector<IntVector3> cells2;

			for (const auto &cluster : clusters) {
				cells2.resize(cluster.size());
				for (std::size_t i = 0; i < cluster.size(); i++) {
					auto p = cluster[i];
					cells2[i] = IntVector3(p.x, p.y, p.z);
					map->Set(p.x, p.y, p.z, false, 0);
				}
				if (listener)
					listener->BlocksFell(cells2);
			}

			createdBlocks.clear();
			destroyedBlocks.clear();
		}

		void World::CreateBlock(spades::IntVector3 pos, spades::IntVector3 color) {
			auto it = destroyedBlocks.find(CellPos(pos.x, pos.y, pos.z));
			if (it != destroyedBlocks.end())
				destroyedBlocks.erase(it);

			createdBlocks[CellPos(pos.x, pos.y, pos.z)] = color;
		}
		void World::DestroyBlock(std::vector<spades::IntVector3> &pos) {
			std::vector<CellPos> cells;
			bool allowToDestroyLand = pos.size() == 1;
			for (size_t i = 0; i < pos.size(); i++) {
				const IntVector3 &p = pos[i];
				if (p.z >= (allowToDestroyLand ? 63 : 62) || p.z < 0 || p.x < 0 || p.y < 0 ||
				    p.x >= map->Width() || p.y >= map->Height())
					continue;

				CellPos cellp(p.x, p.y, p.z);
				auto it = createdBlocks.find(cellp);
				if (it != createdBlocks.end())
					createdBlocks.erase(it);
				destroyedBlocks.insert(cellp);
			}
		}

		World::PlayerPersistent &World::GetPlayerPersistent(int index) {
			SPAssert(index >= 0);
			SPAssert(index < players.size());
			return playerPersistents.at(index);
		}

		std::vector<IntVector3> World::CubeLine(spades::IntVector3 v1, spades::IntVector3 v2,
		                                        int maxLength) {
			SPADES_MARK_FUNCTION_DEBUG();

			IntVector3 c = v1;
			IntVector3 d = v2 - v1;
			long ixi, iyi, izi, dx, dy, dz, dxi, dyi, dzi;
			std::vector<IntVector3> ret;

			int VSID = map->Width();
			SPAssert(VSID == map->Height());

			int MAXZDIM = map->Depth();

			if (d.x < 0)
				ixi = -1;
			else
				ixi = 1;
			if (d.y < 0)
				iyi = -1;
			else
				iyi = 1;
			if (d.z < 0)
				izi = -1;
			else
				izi = 1;

			if ((abs(d.x) >= abs(d.y)) && (abs(d.x) >= abs(d.z))) {
				dxi = 1024;
				dx = 512;
				dyi = (long)(!d.y ? 0x3fffffff / VSID : abs(d.x * 1024 / d.y));
				dy = dyi / 2;
				dzi = (long)(!d.z ? 0x3fffffff / VSID : abs(d.x * 1024 / d.z));
				dz = dzi / 2;
			} else if (abs(d.y) >= abs(d.z)) {
				dyi = 1024;
				dy = 512;
				dxi = (long)(!d.x ? 0x3fffffff / VSID : abs(d.y * 1024 / d.x));
				dx = dxi / 2;
				dzi = (long)(!d.z ? 0x3fffffff / VSID : abs(d.y * 1024 / d.z));
				dz = dzi / 2;
			} else {
				dzi = 1024;
				dz = 512;
				dxi = (long)(!d.x ? 0x3fffffff / VSID : abs(d.z * 1024 / d.x));
				dx = dxi / 2;
				dyi = (long)(!d.y ? 0x3fffffff / VSID : abs(d.z * 1024 / d.y));
				dy = dyi / 2;
			}
			if (ixi >= 0)
				dx = dxi - dx;
			if (iyi >= 0)
				dy = dyi - dy;
			if (izi >= 0)
				dz = dzi - dz;

			while (1) {
				ret.push_back(c);

				if (ret.size() == (size_t)maxLength)
					break;

				if (c.x == v2.x && c.y == v2.y && c.z == v2.z)
					break;

				if ((dz <= dx) && (dz <= dy)) {
					c.z += izi;
					if (c.z < 0 || c.z >= MAXZDIM)
						break;
					dz += dzi;
				} else {
					if (dx < dy) {
						c.x += ixi;
						if ((unsigned long)c.x >= VSID)
							break;
						dx += dxi;
					} else {
						c.y += iyi;
						if ((unsigned long)c.y >= VSID)
							break;
						dy += dyi;
					}
				}
			}

			return ret;
		}

		World::WeaponRayCastResult World::WeaponRayCast(spades::Vector3 startPos,
		                                                spades::Vector3 dir,
		                                                stmp::optional<int> excludePlayerId) {
			WeaponRayCastResult result;
			stmp::optional<int> hitPlayer;
			float hitPlayerDistance = 0.f;
			hitTag_t hitFlag = hit_None;

			for (int i = 0; i < (int)players.size(); i++) {
				const auto &p = players[i];
				if (!p || (excludePlayerId && *excludePlayerId == i))
					continue;
				if (p->GetTeamId() >= 2 || !p->IsAlive())
					continue;
				if (!p->RayCastApprox(startPos, dir))
					continue;

				Player::HitBoxes hb = p->GetHitBoxes();
				Vector3 hitPos;

				if (hb.head.RayCast(startPos, dir, &hitPos)) {
					float dist = (hitPos - startPos).GetLength();
					if (!hitPlayer || dist < hitPlayerDistance) {
						if (hitPlayer != i) {
							hitPlayer = i;
							hitFlag = hit_None;
						}
						hitPlayerDistance = dist;
						hitFlag |= hit_Head;
					}
				}
				if (hb.torso.RayCast(startPos, dir, &hitPos)) {
					float dist = (hitPos - startPos).GetLength();
					if (!hitPlayer || dist < hitPlayerDistance) {
						if (hitPlayer != i) {
							hitPlayer = i;
							hitFlag = hit_None;
						}
						hitPlayerDistance = dist;
						hitFlag |= hit_Torso;
					}
				}
				for (int j = 0; j < 3; j++) {
					if (hb.limbs[j].RayCast(startPos, dir, &hitPos)) {
						float dist = (hitPos - startPos).GetLength();
						if (!hitPlayer || dist < hitPlayerDistance) {
							if (hitPlayer != i) {
								hitPlayer = i;
								hitFlag = hit_None;
							}
							hitPlayerDistance = dist;
							if (j == 2) {
								hitFlag |= hit_Arms;
							} else {
								hitFlag |= hit_Legs;
							}
						}
					}
				}
			}

			// map raycast
			GameMap::RayCastResult res2;
			res2 = map->CastRay2(startPos, dir, 256);

			if (res2.hit &&
			    (!hitPlayer || (res2.hitPos - startPos).GetLength() < hitPlayerDistance)) {
				result.hit = true;
				result.startSolid = res2.startSolid;
				result.hitFlag = hit_None;
				result.blockPos = res2.hitBlock;
				result.hitPos = res2.hitPos;
			} else if (hitPlayer) {
				result.hit = true;
				result.startSolid = false; // FIXME: startSolid for player
				result.playerId = hitPlayer;
				result.hitPos = startPos + dir * hitPlayerDistance;
				result.hitFlag = hitFlag;
			} else {
				result.hit = false;
			}

			return result;
		}

		HitTestDebugger *World::GetHitTestDebugger() {
			if (cg_debugHitTest) {
				if (hitTestDebugger == nullptr) {
					hitTestDebugger = stmp::make_unique<HitTestDebugger>(this);
				}
				return hitTestDebugger.get();
			}
			return nullptr;
		}
	} // namespace client
} // namespace spades
