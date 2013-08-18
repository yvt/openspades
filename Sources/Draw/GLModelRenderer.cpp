//
//  GLModelRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLModelRenderer.h"
#include "GLModel.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		GLModelRenderer::GLModelRenderer(GLRenderer *r):
		device(r->GetGLDevice()), renderer(r){
			SPADES_MARK_FUNCTION();
			
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
			
			models[model->renderId].params.push_back(param);
		}
		
		void GLModelRenderer::RenderShadowMapPass() {
			SPADES_MARK_FUNCTION();
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
			for(size_t i = 0; i < models.size(); i++){
				RenderModel& m = models[i];
				GLModel *model = m.model;
				
				model->RenderSunlightPass(m.params);
			}
		}
		
		void GLModelRenderer::RenderDynamicLightPass(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();
			
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
		}
	}
}
