//
//  Tracer.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/30/13.
//  Copyright (c) 2013 OpenSpades Developers
//

#pragma once

#include "ILocalEntity.h"
#include "../Core/Math.h"

namespace spades {
	namespace client {
		class Client;
		class IImage;
		class Tracer: public ILocalEntity {
			Client *client;
			IImage *image;
			Vector3 startPos, dir;
			float length;
			float curDistance;
			float visibleLength;
			float velocity;
			bool firstUpdate;
		public:
			Tracer(Client *, Vector3 p1, Vector3 p2,
				   float bulletVel);
			virtual ~Tracer();
			
			virtual bool Update(float dt);
			virtual void Render3D();
		};
	}
}