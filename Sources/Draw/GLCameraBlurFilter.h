//
//  GLCameraBlurFilter.h
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLCameraBlurFilter {
			GLRenderer *renderer;
			Matrix4 prevMatrix;
		public:
			GLCameraBlurFilter(GLRenderer *);
			GLColorBuffer Filter(GLColorBuffer);
			
		};
	}
}
