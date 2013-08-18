//
//  GLModel.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IModel.h"
#include "../Client/IRenderer.h"
#include <vector>
#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		class GLModelRenderer;
		struct GLShadowMapRenderParam;
		class GLModel: public client::IModel {
			friend class GLModelRenderer;
		public:
			GLModel();
			
			/** Renders for shadow map */
			virtual void RenderShadowMapPass(std::vector<client::ModelRenderParam> params) = 0;
			
			/** Renders only in depth buffer (optional) */
			virtual void Prerender(std::vector<client::ModelRenderParam> params) {}
			
			/** Renders sunlighted solid geometry */
			virtual void RenderSunlightPass(std::vector<client::ModelRenderParam> params) = 0;
			
			/** Adds dynamic light */
			virtual void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params, std::vector<GLDynamicLight> lights) = 0;
			
		private:
			// members used when rendering by GLModelRenderer
			int renderId;
		};
	}
};
