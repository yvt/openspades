//
//  GLBloomFilter.h
//  OpenSpades
//
//  Created by yvt on 7/21/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLBloomFilter {
			GLRenderer *renderer;
		public:
			GLBloomFilter(GLRenderer *);
			GLColorBuffer Filter(GLColorBuffer);
		};
	}
}
