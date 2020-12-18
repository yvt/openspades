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
#include <Client/IModel.h>
#include <Client/IRenderer.h>

namespace spades {
	namespace draw {
		class GLModelRenderer;
		struct GLShadowMapRenderParam;
		class GLModel : public client::IModel {
			friend class GLModelRenderer;

		public:
			GLModel();

			/** Renders for shadow map */
			virtual void RenderShadowMapPass(std::vector<client::ModelRenderParam> params) = 0;

			/** Renders only in depth buffer (optional) */
			virtual void Prerender(std::vector<client::ModelRenderParam> params,
			                       bool ghostPass) = 0;

			/** Renders sunlighted solid geometry */
			virtual void RenderSunlightPass(std::vector<client::ModelRenderParam> params,
			                                bool ghostPass,
			                                bool farRender) = 0;

			/** Adds dynamic light */
			virtual void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params,
			                                    std::vector<GLDynamicLight> lights,
			                                    bool farRender) = 0;

			virtual void RenderOutlinesPass(std::vector<client::ModelRenderParam> params,
			                                Vector3 outlineColor, bool fog, bool farRender) = 0;
			virtual void RenderOccludedPass(std::vector<client::ModelRenderParam> params,
			                                bool farRender) = 0;
			virtual void RenderOcclusionTestPass(std::vector<client::ModelRenderParam> params,
			                                     bool farRender) = 0;

		private:
			// members used when rendering by GLModelRenderer
			int renderId;
		};
	}
};
