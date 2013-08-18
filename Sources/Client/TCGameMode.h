//
//  TCGameMode.h
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGameMode.h"
#include "../Core/Math.h"
#include "../Core/Debug.h"
#include <vector>

namespace spades {
	namespace client {
		class World;
		class TCGameMode: public IGameMode {
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
			struct Team {
			};
			int captureLimit;
		private:
			World *world;
			Team teams[2];
			std::vector<Territory> territories;
		public:
			TCGameMode(World *);
			virtual ~TCGameMode();
			
			Team& GetTeam(int t);
			
			int GetNumTerritories() const{
				return (int)territories.size();
			}
			Territory *GetTerritory(int index) {
				SPADES_MARK_FUNCTION();
				SPAssert(index >= 0);
				SPAssert(index < GetNumTerritories());
				return &(territories[index]);
			}
			
			void AddTerritory(const Territory&);
		};
	}
}

