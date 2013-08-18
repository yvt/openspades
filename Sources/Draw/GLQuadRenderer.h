//
//  GLQuadRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"

namespace spades {
	namespace draw {
		
		/** draws a quad for post filters.
		 * load program by yourself, and GLQuadRenderer does 
		 * setting vertex attributes and drawing. */
		class GLQuadRenderer {
			IGLDevice *device;
			
			IGLDevice::UInteger attrIndex;
		public:
			GLQuadRenderer(IGLDevice *);
			~GLQuadRenderer();
			
			/** specifies the index of an attribute that
			 * indicates the coordinate in [0, 1] */
			void SetCoordAttributeIndex(IGLDevice::UInteger);
			
			void Draw();
		};
	}
}