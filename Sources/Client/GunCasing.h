//
//  GunCasing.h
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "ILocalEntity.h"
#include "IModel.h"
#include "../Core/Math.h"

namespace spades {
	namespace client {
		class IRenderer;
		class Client;
		class IAudioChunk;
		class GunCasing: public ILocalEntity {
			Client *client;
			IRenderer *renderer;
			IModel *model;
			Matrix4 matrix;
			Vector3 rotAxis;
			Vector3 vel;
			
			IAudioChunk *dropSound;
			IAudioChunk *waterSound;
			
			bool onGround;
			IntVector3 groundPos;
			float groundTime;
			float rotSpeed;
		public:
			GunCasing(Client *client,
					  IModel *model,
					  IAudioChunk *dropSound,
					  IAudioChunk *waterSound,
					  Vector3 pos,
					  Vector3 dir,
					  Vector3 flyDir);
			virtual ~GunCasing();
			virtual bool Update(float dt);
			virtual void Render3D();
		};
	}
}