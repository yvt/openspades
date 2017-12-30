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

#include "CTFGameMode.h"
#include <Core/Debug.h>

#include "World.h"
#include "Player.h"

namespace spades {
	namespace client {
		CTFGameMode::CTFGameMode() : IGameMode(m_CTF) { SPADES_MARK_FUNCTION(); }
		CTFGameMode::~CTFGameMode() { SPADES_MARK_FUNCTION(); }

		CTFGameMode::Team &CTFGameMode::GetTeam(int t) {
			SPADES_MARK_FUNCTION();
			SPAssert(t >= 0);
			SPAssert(t < 2);
			return teams[t];
		}

		bool CTFGameMode::PlayerHasIntel(World &world, Player &player) {
			if (player.IsSpectator()) {
				return false;
			}

			auto &team = teams[player.GetTeamId()];
			return team.hasIntel && team.carrier == player.GetId();
		}
	}
}
