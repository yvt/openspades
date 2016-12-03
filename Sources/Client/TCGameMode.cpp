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

#include "TCGameMode.h"
#include <Core/Debug.h>
#include "World.h"

namespace spades {
	namespace client {
		TCGameMode::TCGameMode(World *w) : IGameMode(m_TC), world(w) { SPADES_MARK_FUNCTION(); }
		TCGameMode::~TCGameMode() { SPADES_MARK_FUNCTION(); }

		TCGameMode::Team &TCGameMode::GetTeam(int t) {
			SPADES_MARK_FUNCTION();
			SPAssert(t >= 0);
			SPAssert(t < 2);
			return teams[t];
		}

		void TCGameMode::AddTerritory(const spades::client::TCGameMode::Territory &t) {
			territories.push_back(t);
		}

		float TCGameMode::Territory::GetProgress() {
			float dt = mode->world->GetTime() - progressStartTime;
			float prog = progressBasePos;
			prog += progressRate * dt;
			if (prog < 0.f)
				prog = 0.f;
			if (prog > 1.f)
				prog = 1.f;
			return prog;
		}
	}
}
