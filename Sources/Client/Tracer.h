//
//  Tracer.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/30/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <Core/Math.h>
#include "ILocalEntity.h"

namespace spades {
	namespace client {
		class Client;
		class IImage;
		class Tracer : public ILocalEntity {
			Client *client;
			IImage *image;
			Vector3 startPos, dir;
			float length;
			float curDistance;
			float visibleLength;
			float velocity;
			bool firstUpdate;

		public:
			Tracer(Client *, Vector3 p1, Vector3 p2, float bulletVel);
			~Tracer();

			bool Update(float dt) override;
			void Render3D() override;
		};
	}
}