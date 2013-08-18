//
//  GLLensFilter.h
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#pragma once

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLLensFilter {
			GLRenderer *renderer;
		public:
			GLLensFilter(GLRenderer *);
			GLColorBuffer Filter(GLColorBuffer);
		};
	}
}
