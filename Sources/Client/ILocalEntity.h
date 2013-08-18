//
//  ILocalEntity.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

namespace spades {
	namespace client {
		class ILocalEntity {
		public:
			virtual ~ILocalEntity(){}
			/** @return false if this entity should be removed from the scene. */
			virtual bool Update(float dt) = 0;
			virtual void Render3D(){}
			virtual void Render2D(){}
		};
	}
}
