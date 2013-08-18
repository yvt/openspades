//
//  IGameMapListener.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

namespace spades {
	namespace client {
		class GameMap;
		class IGameMapListener{
		public:
			virtual void GameMapChanged(int x, int y, int z, GameMap *) = 0;
		};
	}
}
