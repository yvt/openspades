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

#pragma once

#include <Core/RefCountedObject.h>
#include "Constants.h"
#include <Client/GameMap.h>
#include <Core/TMPUtils.h>
#include <map>
#include <set>

namespace spades { namespace client {
	class GameMapWrapper;
} }

namespace spades { namespace game {
	
	/** World-global parameters. */
	struct WorldParameters {
		
		float playerJumpVelocity;
		float fallDamageVelocity;
		float fatalFallDamageVelocity;
		float fogDistance;
		Vector3 fogColor;
		
		WorldParameters();
		
		std::map<std::string, std::string> Serialize() const;
		void Update(const std::string& key,
					const std::string& value);
	};
	
	
	class Entity;
	class WorldListener;
	class Player;
	
	class PlayerEntity;
	
	
	class World: public RefCountedObject
	{
		Timepoint currentTime;
		WorldParameters params;
		Handle<client::GameMap> gameMap;
		std::unique_ptr<client::GameMapWrapper> gameMapWrapper;
		std::map<uint32_t, Handle<Entity>> entities;
		std::map<uint32_t, Handle<Player>> players;
		std::set<WorldListener *> listeners;
		stmp::optional<uint32_t> localPlayerId;
		
		struct IntVectorComparator {
			inline bool operator ()
			(const IntVector3& a,
			 const IntVector3& b) const {
				if (a.x < b.x) return true;
				if (a.x > b.x) return false;
				if (a.y < b.y) return true;
				if (a.y > b.y) return false;
				return a.z < b.z;
			}
		};
		std::map<IntVector3, MapEdit,
		IntVectorComparator> mapEdits;
		
	public:
		World(const WorldParameters& params,
			  client::GameMap *);
		~World();
		
		void AddListener(WorldListener *);
		void RemoveListener(WorldListener *);
		
		Entity *FindEntity(uint32_t);
		/** adds an entity to the world.
		 * server only. client networking code might call this. */
		void LinkEntity(Entity *,
						stmp::optional<uint32_t> entityId = stmp::optional<uint32_t>());
		/** removes an entity from the world.
		 * server only. client networking code might call this. */
		void UnlinkEntity(Entity *);
		std::vector<Entity *> GetAllEntities();
		
		Player *FindPlayer(uint32_t);
		void CreatePlayer(Player *,
						  stmp::optional<uint32_t> playerId = stmp::optional<uint32_t>());
		void RemovePlayer(Player *);
		std::vector<Player *> GetAllPlayers();
		
		/** server only. */
		void CreateBlock(const IntVector3&,
						 uint32_t color);
		/** server only. */
		void DestroyBlock(const IntVector3&);
		/** intended to be called only by server.
		 * */
		void FlushMapEdits();
		bool IsValidMapCoord(const IntVector3&);
		
		void ReceivedMapEdits(const std::vector<MapEdit>&);
		
		Timepoint GetCurrentTime() const { return currentTime; }
		
		void Advance(Duration);
		
		/** Returns global gravity vector.
		 * @return Gravity in units/sec^2. */
		Vector3 GetGravity();
		
		bool IsWaterAt(const Vector3&);
		
		bool IsLocalHostServer();
		bool IsLocalHostClient();
		stmp::optional<uint32_t> GetLocalPlayerId();
		PlayerEntity *GetLocalPlayerEntity();
		Player *GetLocalPlayer();
		void SetLocalPlayerId(stmp::optional<uint32_t>);
		
		/** Returns game map.
		 * Be aware that modifying this map directly
		 * does result in a serious inconsistency problem.
		 * Always use map edit APIs. */
		client::GameMap *GetGameMap() { return gameMap; }
		
		const WorldParameters& GetParameters() const
		{ return params; }
		
	};
	
	class WorldListener
	{
	public:
		virtual ~WorldListener() { }
		
		virtual void EntityLinked(World&, Entity *) { }
		virtual void EntityUnlinked(World&, Entity *) { }
		
		virtual void PlayerCreated(World&, Player *) { }
		virtual void PlayerRemoved(World&, Player *) { }
		
		virtual void BlockCreated(const IntVector3&,
								  BlockCreateType) { }
		virtual void BlockUpdated(const IntVector3&,
								  BlockCreateType) { }
		virtual void BlockDestroyed(const IntVector3&,
									BlockDestroyType) { }
		
		virtual void BlocksFalling(const std::vector<IntVector3>&) { }
		
		virtual void FlushMapEdits(const std::vector<MapEdit>&) { }
		
	};
	
} }

