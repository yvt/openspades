//
//  SceneDefinition.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//
#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		struct SceneDefinition {
			int viewportLeft,  viewportTop;
			int viewportWidth, viewportHeight;
			float fovX, fovY;
			Vector3 viewOrigin;
			Vector3 viewAxis[3];
			float zNear, zFar;
			bool skipWorld;
			
			unsigned int time;
			
			bool denyCameraBlur;
		};
	}
}
