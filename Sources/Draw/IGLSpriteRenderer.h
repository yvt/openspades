//
//  IGLSpriteRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace draw {
		class GLImage;
		class IGLSpriteRenderer {
		public:
			virtual ~IGLSpriteRenderer(){}
			virtual void Add(GLImage *img, Vector3 center,
					 float rad, float ang, Vector4 color) = 0;
			virtual void Clear() = 0;
			virtual void Render() = 0;
		};
	}
}