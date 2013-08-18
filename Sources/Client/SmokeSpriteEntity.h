//
//  SmokeSpriteEntity.h
//  OpenSpades
//
//  Created by yvt on 7/21/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "ParticleSpriteEntity.h"

namespace spades{
	namespace client{
		class SmokeSpriteEntity: public ParticleSpriteEntity{
			float frame;
			float fps;
		public:
			SmokeSpriteEntity(Client *cli, Vector4 color,
							  float fps);
			virtual bool Update(float dt);
		};
	}
}