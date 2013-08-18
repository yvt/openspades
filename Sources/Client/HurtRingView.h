//
//  HurtRingView.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include <list>

namespace spades {
	namespace client{
		class IRenderer;
		class Client;
		class IImage;
		
		class HurtRingView {
			Client *client;
			IRenderer *renderer;
			IImage *image;
			
			struct Item{
				Vector3 dir;
				float fade;
			};
			
			std::list<Item> items;
			
		public:
			HurtRingView(Client *);
			~HurtRingView();
			
			void ClearAll();
			void Add(Vector3);
			
			void Update(float dt);
			void Draw();
		};
	}
}
