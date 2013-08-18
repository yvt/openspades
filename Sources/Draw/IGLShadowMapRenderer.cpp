//
//  IGLShadowMapRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "IGLShadowMapRenderer.h"
#include "GLRenderer.h"
#include "GLModelRenderer.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		IGLShadowMapRenderer::IGLShadowMapRenderer(GLRenderer *renderer):
		renderer(renderer){}
		void IGLShadowMapRenderer::RenderShadowMapPass() {
			SPADES_MARK_FUNCTION();
			renderer->modelRenderer->RenderShadowMapPass();
		}
	}
}