//
//  IGLShadowMapRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace draw {
		
		class GLRenderer;
		
		struct GLShadowMapRenderParam {
			Matrix4 matrix;
			
			virtual ~GLShadowMapRenderParam(){}
			
		};
		
		class IGLShadowMapRenderer{
			GLRenderer *renderer;
		protected:
			void RenderShadowMapPass();
		public:
			IGLShadowMapRenderer(GLRenderer *);
			virtual ~IGLShadowMapRenderer(){}
			
			GLRenderer *GetRenderer() { return renderer; }
			
			virtual void Render() = 0;
			
			virtual bool Cull(const AABB3&) = 0;
			virtual bool SphereCull(const Vector3& center, float rad) = 0;
		};
	}
}