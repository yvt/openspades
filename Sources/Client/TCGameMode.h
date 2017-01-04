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

#include <vector>

#include "IGameMode.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace client {
		class World;
		class TCGameMode : public IGameMode {
		public:
			struct Territory {
				TCGameMode *mode;

				Vector3 pos;
				/** team that owns this territory, or 2 if this territory is currently neutral.*/
				int ownerTeamId;

				/** team trying to capture this, or -1 if no team have tried */
				int capturingTeamId;

				float progressBasePos;
				float progressRate;
				float progressStartTime;

				/** gets capture progress of this territory.
				 * 0 = ownerTeamId is capturing, 1 = 1-ownerTeamId has captured this. */
				float GetProgress();
			};
			struct Team {};
			int captureLimit;

		private:
			World *world;
			Team teams[2];
			std::vector<Territory> territories;

		public:
			TCGameMode(World *);
			~TCGameMode();

			Team &GetTeam(int t);

			int GetNumTerritories() const { return (int)territories.size(); }
			Territory *GetTerritory(int index) {
				SPADES_MARK_FUNCTION();
				SPAssert(index >= 0);
				SPAssert(index < GetNumTerritories());
				return &(territories[index]);
			}

			void AddTerritory(const Territory &);
		};
	}
}
