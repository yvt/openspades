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
#include "GLProfiler.h"
#include "GLRenderer.h"
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		GLModelRenderer::GLModelRenderer(GLRenderer *r) : renderer(r), device(r->GetGLDevice()) {
			SPADES_MARK_FUNCTION();

			// Create the queries, default them with empty result.
			for (auto &playerVisibilityQuerie : playerVisibilityQueries) {
				playerVisibilityQuerie = device->GenQuery();
				device->BeginQuery(IGLDevice::SamplesPassed, playerVisibilityQuerie);
				device->EndQuery(IGLDevice::SamplesPassed);
			}
			device->Flush();
			
			modelCount = 0;
		}

		GLModelRenderer::~GLModelRenderer() {
			SPADES_MARK_FUNCTION();

			// Free occlusion query objects.
			for (auto playerVisibilityQuerie : playerVisibilityQueries) {
				device->DeleteQuery(playerVisibilityQuerie);
			}

			Clear();
		}

		void GLModelRenderer::AddModel(GLModel *model, const client::ModelRenderParam &param) {
			SPADES_MARK_FUNCTION();
			if (model->renderId == -1) {
				model->renderId = (int)models.size();
				RenderModel m;
				m.model = model;
				model->AddRef();
				models.push_back(m);
			}
			modelCount++;
			models[model->renderId].params.push_back(param);
		}

		void GLModelRenderer::RenderShadowMapPass() {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			int numModels = 0;
			for (size_t i = 0; i < models.size(); i++) {
				RenderModel &m = models[i];
				GLModel *model = m.model;
				model->RenderShadowMapPass(m.params);
				numModels += (int)m.params.size();
			}
#if 0
			printf("Model types: %d, Number of models: %d\n",
				   (int)models.size(), numModels);
#endif
		}

		void GLModelRenderer::Prerender(bool ghostPass) {
			device->ColorMask(false, false, false, false);

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			int numModels = 0;
			for (size_t i = 0; i < models.size(); i++) {
				RenderModel &m = models[i];
				GLModel *model = m.model;
				model->Prerender(m.params, ghostPass);
				numModels += (int)m.params.size();
			}
			device->ColorMask(true, true, true, true);
		}

		void GLModelRenderer::RenderSunlightPass(bool ghostPass) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (size_t i = 0; i < models.size(); i++) {
				RenderModel &m = models[i];
				GLModel *model = m.model;

				model->RenderSunlightPass(m.params, ghostPass, false);
			}
		}

		void GLModelRenderer::RenderDynamicLightPass(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			if (!lights.empty()) {

				for (size_t i = 0; i < models.size(); i++) {
					RenderModel &m = models[i];
					GLModel *model = m.model;

					model->RenderDynamicLightPass(m.params, lights, false);
				}
			}
		}

		void GLModelRenderer::RenderOutlinesPass() {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				auto *model = m.model;

				model->RenderOutlinesPass(m.params, Vector3(0.0f, 0.0f, 0.0f), true, false);
			}
		}

		void GLModelRenderer::DetermineVisiblePlayers(bool visiblePlayers[]) {
			SPADES_MARK_FUNCTION();
			// determine player visbility via the last frame
			for (int i = 0; i < 32; ++i) {
				const int iSamplesPassed = device->GetQueryObjectUInteger(playerVisibilityQueries[i],
				                                                          IGLDevice::QueryResult);
				visiblePlayers[i] = (iSamplesPassed > 0);
			}
			// set up the occlusion query
			device->ColorMask(false, false, false, false);
			device->DepthMask(false);
			// iterate every player and get the new occlusion query going
			for (int i = 0; i < 32; ++i) {
				device->BeginQuery(IGLDevice::SamplesPassed, playerVisibilityQueries[i]);
				for (auto &m : models) {
					std::vector<client::ModelRenderParam> playerParams;
					for (auto p : m.params) {
						if (p.playerID == i) {
							playerParams.push_back(p);
						}
					}
					m.model->RenderOcclusionTestPass(playerParams, false);
				}
				device->EndQuery(IGLDevice::SamplesPassed);
			}
			// end with query stuff
			device->ColorMask(true, true, true, true);
			device->DepthMask(true);
		}

		void GLModelRenderer::RenderSunlightPassNoPlayers() {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> params;
				for (auto p : m.params) {
					if (p.playerID == -1) {
						params.push_back(p);
					}
				}
				m.model->RenderSunlightPass(params, false, false);
			}
		}

		void GLModelRenderer::RenderDynamicLightPassNoPlayers(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> params;
				for (auto p : m.params) {
					if (p.playerID == -1) {
						params.push_back(p);
					}
				}
				m.model->RenderDynamicLightPass(params, lights, false);
			}
		}
		void GLModelRenderer::RenderSunlightPassVisiblePlayers(bool visiblePlayers[]) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> params;
				for (auto p : m.params) {
					if (p.playerID != -1 && visiblePlayers[p.playerID]) {
						params.push_back(p);
					}
				}
				m.model->RenderSunlightPass(params, false, true);
			}
		}
		void
		GLModelRenderer::RenderDynamicLightPassVisiblePlayers(bool visiblePlayers[],
		                                                      std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> params;
				for (auto p : m.params) {
					if (p.playerID != -1 && visiblePlayers[p.playerID]) {
						params.push_back(p);
					}
				}
				m.model->RenderDynamicLightPass(params, lights, true);
			}
		}
		void GLModelRenderer::RenderOccludedPlayers(bool visiblePlayers[]) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> params;
				for (auto p : m.params) {
					if (p.playerID != -1 && !visiblePlayers[p.playerID]) {
						params.push_back(p);
					}
				}
				m.model->RenderOccludedPass(params, true);
			}
		}

		void GLModelRenderer::RenderPlayerVisibilityOutlines(bool visiblePlayers[]) {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(),
			                             "Model [%d model(s), %d unique model type(s)]", modelCount,
			                             (int)models.size());

			for (auto &m : models) {
				std::vector<client::ModelRenderParam> visibleTeam0;
				std::vector<client::ModelRenderParam> nonVisibleTeam0;
				std::vector<client::ModelRenderParam> visibleTeam1;
				std::vector<client::ModelRenderParam> nonVisibleTeam1;
				for (auto p : m.params) {
					if (p.playerID != -1) {
						if (visiblePlayers[p.playerID]) {
							if (p.teamId == 0) {
								visibleTeam0.push_back(p);
							} else {
								visibleTeam1.push_back(p);
							}
						} else {
							if (p.teamId == 0) {
								nonVisibleTeam0.push_back(p);
							} else {
								nonVisibleTeam1.push_back(p);
							}
						}
					}
				}

				auto team0Col = client::Client::TeamCol(0);
				auto team1Col = client::Client::TeamCol(1);

				const auto nv0 = team0Col * 0.63;
				const auto nv1 = team1Col * 0.63;
				const auto v0 = team0Col;
				const auto v1 = team1Col;

				m.model->RenderOutlinesPass(nonVisibleTeam0, nv0, false, true);
				m.model->RenderOutlinesPass(visibleTeam0, v0, false, true);
				m.model->RenderOutlinesPass(nonVisibleTeam1, nv1, false, true);
				m.model->RenderOutlinesPass(visibleTeam1, v1, false, true);
			}
		}

		void GLModelRenderer::Clear() {
			// last phase: clear scene
			for (size_t i = 0; i < models.size(); i++) {
				models[i].model->renderId = -1;
				models[i].model->Release();
			}
			models.clear();

			modelCount = 0;
		}
	}
}
