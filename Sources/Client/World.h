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

#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>

#include "GameMapWrapper.h"
#include "PhysicsConstants.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace client {
		class GameMap;
		class GameMapWrapper;
		class Player;
		class IWorldListener;
		class Grenade;
		class IGameMode;
		class Client; // FIXME: for debug
		class HitTestDebugger;
		struct GameProperties;
		class World {
			friend class Client; // FIXME: for debug
		public:
			struct Team {
				IntVector3 color;
				std::string name;
			};
			struct PlayerPersistent {
				std::string name;
				int kills;
				PlayerPersistent() : kills(0) { ; }
			};

		private:
			IWorldListener *listener;

			IGameMode *mode;

			GameMap *map;
			GameMapWrapper *mapWrapper;
			float time;
			IntVector3 fogColor;
			Team teams[3];

			std::shared_ptr<GameProperties> gameProperties;

			std::vector<Player *> players;
			std::vector<PlayerPersistent> playerPersistents;
			int localPlayerIndex;

			std::list<Grenade *> grenades;
			std::unique_ptr<HitTestDebugger> hitTestDebugger;

			std::unordered_map<CellPos, spades::IntVector3, CellPosHash> createdBlocks;
			std::unordered_set<CellPos, CellPosHash> destroyedBlocks;

			std::multimap<float, IntVector3> blockRegenerationQueue;
			std::unordered_map<IntVector3, std::multimap<float, IntVector3>::iterator>
			  blockRegenerationQueueMap;

			void ApplyBlockActions();

		public:
			World(const std::shared_ptr<GameProperties>&);
			~World();
			GameMap *GetMap() { return map; }
			GameMapWrapper *GetMapWrapper() { return mapWrapper; }
			float GetTime() { return time; }

			/** Returns a non-null reference to `GameProperties`. */
			const std::shared_ptr<GameProperties> &GetGameProperties() { return gameProperties; }

			void SetMap(GameMap *);

			IntVector3 GetFogColor() { return fogColor; }
			void SetFogColor(IntVector3 v) { fogColor = v; }

			void Advance(float dt);

			void AddGrenade(Grenade *);
			std::vector<Grenade *> GetAllGrenades();

			void MarkBlockForRegeneration(const IntVector3 &blockLocation);
			void UnmarkBlockForRegeneration(const IntVector3 &blockLocation);

			std::vector<IntVector3> CubeLine(IntVector3 v1, IntVector3 v2, int maxLength);

			Player *GetPlayer(unsigned int i) {
				// SPAssert(i >= 0);	lm: unsigned cannot be smaller than 0 :)
				SPAssert(i < players.size());
				return players[i];
			}

			void SetPlayer(int i, Player *p);

			IGameMode *GetMode() { return mode; }
			void SetMode(IGameMode *);

			Team &GetTeam(int t) {
				if (t >= 2 || t < 0) // spectator
					return teams[2];
				return teams[t];
			}

			PlayerPersistent &GetPlayerPersistent(int index);

			void CreateBlock(IntVector3 pos, IntVector3 color);
			void DestroyBlock(std::vector<IntVector3> &pos);

			struct WeaponRayCastResult {
				bool hit, startSolid;
				Player *player;
				IntVector3 blockPos;
				Vector3 hitPos;
				hitTag_t hitFlag;
			};

			WeaponRayCastResult WeaponRayCast(Vector3 startPos, Vector3 dir, Player *exclude);

			size_t GetNumPlayerSlots() { return players.size(); }

			size_t GetNumPlayers();

			int GetLocalPlayerIndex() { return localPlayerIndex; }

			void SetLocalPlayerIndex(int p) { localPlayerIndex = p; }

			Player *GetLocalPlayer() {
				if (GetLocalPlayerIndex() == -1)
					return NULL;
				return GetPlayer(GetLocalPlayerIndex());
			}

			HitTestDebugger *GetHitTestDebugger();

			void SetListener(IWorldListener *l) { listener = l; }
			IWorldListener *GetListener() { return listener; }
		};
	}
}
