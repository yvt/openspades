//
//  GLLensFlareFilter.h
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
		class GLLensFlareFilter {
			GLRenderer *renderer;
			GLColorBuffer Blur(GLColorBuffer, float spread = 1.f);
		public:
			GLLensFlareFilter(GLRenderer *);
			void Draw();
		};
	}
}

