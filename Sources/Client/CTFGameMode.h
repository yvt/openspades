//
//  CTFGameMode.h
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once
#include "IGameMode.h"
#include "../Core/Math.h"

namespace spades {
	namespace client {
		class CTFGameMode: public IGameMode {
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
			virtual ~CTFGameMode();
			
			Team& GetTeam(int t);
			int GetCaptureLimit() { return captureLimit; }
			void SetCaptureLimit(int v){ captureLimit = v; }
		};
	}
}
