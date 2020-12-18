/*
 Copyright (c) 2013 yvt

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

#include <vector>

#include "GLDynamicLight.h"
#include "IGLDevice.h"
#include <Client/IModel.h>
#include <Client/IRenderer.h>

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

			unsigned int playerVisibilityQueries[32];

		public:
			GLModelRenderer(GLRenderer *);
			~GLModelRenderer();

			void AddModel(GLModel *model, const client::ModelRenderParam &param);

			void RenderShadowMapPass();

			void Prerender(bool ghostPass);
			void RenderSunlightPass(bool ghostPass);
			void RenderDynamicLightPass(std::vector<GLDynamicLight> lights);

			void RenderOutlinesPass();
			void DetermineVisiblePlayers(bool visiblePlayers[]);
			void RenderSunlightPassNoPlayers();
			void RenderDynamicLightPassNoPlayers(std::vector<GLDynamicLight> lights);
			void RenderSunlightPassVisiblePlayers(bool visiblePlayers[]);
			void RenderDynamicLightPassVisiblePlayers(bool visiblePlayers[],
			                                          std::vector<GLDynamicLight> lights);
			void RenderOccludedPlayers(bool visiblePlayers[]);
			void RenderPlayerVisibilityOutlines(bool visiblePlayers[]);

			void Clear();
		};
	}
}
