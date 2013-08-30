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
#include "GLModelRenderer.h"
#include "GLModel.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"
#include "GLProfiler.h"

namespace spades {
	namespace draw {
		GLModelRenderer::GLModelRenderer(GLRenderer *r):
		device(r->GetGLDevice()), renderer(r){
			SPADES_MARK_FUNCTION();
			modelCount = 0;
		}
		
		GLModelRenderer::~GLModelRenderer() {
			
			SPADES_MARK_FUNCTION();
		}
		
		void GLModelRenderer::AddModel(GLModel *model,
									   const client::ModelRenderParam &param) {
			SPADES_MARK_FUNCTION();
			if(model->renderId == -1){
				model->renderId = (int)models.size();
				RenderModel m;
				m.model = model;
				models.push_back(m);
			}
			modelCount++;
			models[model->renderId].params.push_back(param);
		}
		
		void GLModelRenderer::RenderShadowMapPass() {
			SPADES_MARK_FUNCTION();
			
			GLProfiler profiler(device, "Model [%d model(s), %d unique model type(s)]", modelCount, (int)models.size());
			
			int numModels = 0;
			for(size_t i = 0; i < models.size(); i++){
				RenderModel& m = models[i];
				GLModel *model = m.model;
				model->RenderShadowMapPass(m.params);
				numModels += (int)m.params.size();
			}
#if 0
			printf("Model types: %d, Number of models: %d\n",
				   (int)models.size(), numModels);
#endif
		}
		
		void GLModelRenderer::Prerender() {
			SPADES_MARK_FUNCTION();
			
		}
		
		void GLModelRenderer::RenderSunlightPass() {
			SPADES_MARK_FUNCTION();
			
			GLProfiler profiler(device, "Model [%d model(s), %d unique model type(s)]", modelCount, (int)models.size());
			
			for(size_t i = 0; i < models.size(); i++){
				RenderModel& m = models[i];
				GLModel *model = m.model;
				
				model->RenderSunlightPass(m.params);
			}
		}
		
		void GLModelRenderer::RenderDynamicLightPass(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();
			
			GLProfiler profiler(device, "Model [%d model(s), %d unique model type(s)]", modelCount, (int)models.size());
			
			if(!lights.empty()){
				
				for(size_t i = 0; i < models.size(); i++){
					RenderModel& m = models[i];
					GLModel *model = m.model;
					
					model->RenderDynamicLightPass(m.params, lights);
				}
					
			}
			
			// last phase: clear scene
			for(size_t i = 0; i < models.size(); i++){
				models[i].model->renderId = -1;
			}
			models.clear();
			
			modelCount = 0;
		}
	}
}
