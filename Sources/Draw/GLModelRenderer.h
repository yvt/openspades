/*
 Copyright (c) 2013 OpenSpades Developers
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */

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
		class GLSparseShadowMapRenderer;
		class GLModelRenderer {
			friend class GLSparseShadowMapRenderer;
			
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
			
			void Clear();
			
		};
	}
}