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
#include "IGameMode.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		class World;
		class Player;

		class CTFGameMode : public IGameMode {
		public:
			struct Team {
				unsigned int score;
				bool hasIntel;
				unsigned int carrier;
				Vector3 flagPos;
				Vector3 basePos;
			};
			int captureLimit;

		private:
			Team teams[2];

		public:
			CTFGameMode();
			~CTFGameMode();

			Team &GetTeam(int t);
			int GetCaptureLimit() { return captureLimit; }
			void SetCaptureLimit(int v) { captureLimit = v; }

			bool PlayerHasIntel(World &world, Player &player);
		};
	}
}
