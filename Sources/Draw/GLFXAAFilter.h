//
//  GLFXAAFilter.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLFXAAFilter {
			GLRenderer *renderer;
		public:
			GLFXAAFilter(GLRenderer *);
			GLColorBuffer Filter(GLColorBuffer);
		};
	}
}
