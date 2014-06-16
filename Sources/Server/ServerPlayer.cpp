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

#include "ServerPlayer.h"

namespace spades { namespace server {
	
	ServerPlayer::ServerPlayer(game::Player& player,
							   Server& server):
	player(player),
	server(server) {
	}
	
	ServerPlayer::~ServerPlayer() {
		
	}
	
	void ServerPlayer::Update(double) {
		SPADES_MARK_FUNCTION();
		// maybe nothing to do here...
	}
	
	void ServerPlayer::SaveForDeltaEncoding() {
		SPADES_MARK_FUNCTION();
		
		lastState = Serialize();
	}
	
	namespace {
		template<class T>
		stmp::optional<T> ComputeDelta
		(const stmp::optional<T>& old,
		 const stmp::optional<T>& cur) {
			if ((cur && !old) ||
				(cur && old && *cur != *old)) {
				return cur;
			}
			return stmp::optional<T>();
		}
	}
	
	stmp::optional<protocol::PlayerUpdateItem>
	ServerPlayer::DeltaSerialize() {
		SPADES_MARK_FUNCTION();
		
		auto current = Serialize();
		protocol::PlayerUpdateItem ret;
		ret.playerId = current.playerId;
		if (!lastState.name) {
			ret.name = current.name;
		}
		ret.flags = ComputeDelta(lastState.flags, current.flags);
		ret.score = ComputeDelta(lastState.score, current.score);
		
		if (!ret.score &&
			!ret.flags) {
			return stmp::optional<protocol::PlayerUpdateItem>();
		}
		
		return ret;
	}
	
	protocol::PlayerUpdateItem
	ServerPlayer::Serialize() {
		SPADES_MARK_FUNCTION();
		
		protocol::PlayerUpdateItem r;
		r.playerId = *player.GetId();
		r.name = player.GetName();
		r.flags = player.GetFlags();
		r.score = player.GetScore();
		return r;
	}
	
} }

