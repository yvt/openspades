//
//  TCGameMode.cpp
//  OpenSpades
//
//  Created by yvt on 8/5/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "TCGameMode.h"
#include "../Core/Debug.h"
#include "World.h"

namespace spades {
	namespace client {
		TCGameMode::TCGameMode(World *w): world(w) {
			SPADES_MARK_FUNCTION();
			
		}
		TCGameMode::~TCGameMode(){
			SPADES_MARK_FUNCTION();
			
		}
		
		TCGameMode::Team& TCGameMode::GetTeam(int t){
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
			if(prog < 0.f)
				prog = 0.f;
			if(prog > 1.f)
				prog = 1.f;
			return prog;
		}
	}
}

