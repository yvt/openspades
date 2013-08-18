//
//  CTFGameMode.cpp
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "CTFGameMode.h"
#include "../Core/Debug.h"

namespace spades {
	namespace client {
		CTFGameMode::CTFGameMode() {
			SPADES_MARK_FUNCTION();
			
		}
		CTFGameMode::~CTFGameMode(){
			SPADES_MARK_FUNCTION();
			
		}
		
		CTFGameMode::Team& CTFGameMode::GetTeam(int t){
			SPADES_MARK_FUNCTION();
			SPAssert(t >= 0);
			SPAssert(t < 2);
			return teams[t];
		}
	}
}
