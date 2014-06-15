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
#include <Core/TMPUtils.h>
#include <set>

namespace spades { namespace game {
	
	class World;
	class PlayerEntity;
	class PlayerListener;
	
	class Player: public RefCountedObject
	{
		World& world;
		stmp::optional<uint32_t> playerId;
		std::set<PlayerListener *> listeners;
		
		std::string name;
		PlayerFlags flags;
		uint32_t score = 0;
	protected:
		~Player();
	public:
		Player(World&,
			   const std::string& name);
		
		void AddListener(PlayerListener *);
		void RemoveListener(PlayerListener *);
		
		World& GetWorld() const { return world; }
		
		std::string GetName() const { return name; }
		
		uint32_t GetScore() const { return score; }
		void SetScore(uint32_t s) { score = s; }
		
		PlayerFlags& GetFlags() { return flags; }
		
		stmp::optional<uint32_t> GetId() const { return playerId; }
		void SetId(stmp::optional<uint32_t>);
		
		PlayerEntity *GetEntity();
		void Spawn();
	};
	
	class PlayerListener {
		friend class Player;
	public:
		virtual ~PlayerListener() { }
	protected:
		virtual void PlayerRemoved(Player&) { }
	};
	
	
	
} }
