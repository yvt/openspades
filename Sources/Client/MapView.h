//
//  MapView.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		class Client;
		class IRenderer;
		class IImage;
		class MapView {
			Client *client;
			IRenderer *renderer;
			
			int scaleMode;
			float actualScale;
			float lastScale; // used for animation
			
			float zoomState;
			bool zoomed;
			
			bool largeMap;
			
			AABB2 inRect;
			AABB2 outRect;
			
			void DrawIcon(Vector3 pos,
						  IImage *img,
						  float rotation);
		public:
			MapView(Client *, bool largeMap);
			~MapView();
			
			void Update(float dt);
			void SwitchScale();
			void ToggleZoom();
			
			void Draw();
		};
	}
}