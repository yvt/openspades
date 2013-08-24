//
//  GLModelRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IModel.h"
#include "../Client/IRenderer.h"
#include <vector>
#include "IGLDevice.h"
#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLModel;
		class GLModelRenderer {
			GLRenderer *renderer;
			IGLDevice *device;
			
			struct RenderModel {
				GLModel *model;
				std::vector<client::ModelRenderParam> params;
			};
			
			std::vector<RenderModel> models;
			int modelCount;
			
		public:
			GLModelRenderer(GLRenderer *);
			~GLModelRenderer();
			
			void AddModel(GLModel *model,
						  const client::ModelRenderParam& param);
			
			void RenderShadowMapPass();
			
			void Prerender();
			void RenderSunlightPass();
			void RenderDynamicLightPass(std::vector<GLDynamicLight> lights);
			
		};
	}
}