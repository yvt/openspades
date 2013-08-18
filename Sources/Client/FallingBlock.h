//
//  FallingBlock.h
//  OpenSpades
//
//  Created by yvt on 7/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include <vector>
#include "../Core/VoxelModel.h"
#include "ILocalEntity.h"

namespace spades {
	namespace client {
		class Client;
		class IModel;
		
		class FallingBlock: public ILocalEntity {
			Client *client;
			IModel *model;
			VoxelModel *vmodel;
			Matrix4 matrix;
			Matrix4 lastMatrix;
			float time;
			int numBlocks;
		public:
			FallingBlock(Client *, std::vector<IntVector3> blocks);
			virtual ~FallingBlock();
			
			virtual bool Update(float dt);
			virtual void Render3D();
		};
	}
}