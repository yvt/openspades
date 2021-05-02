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

#include <cstdarg>
#include <cstdlib>

#include "GLAmbientShadowRenderer.h"
#include "GLAutoExposureFilter.h"
#include "GLBloomFilter.h"
#include "GLColorCorrectionFilter.h"
#include "GLDepthOfFieldFilter.h"
#include "GLFXAAFilter.h"
#include "GLFlatMapRenderer.h"
#include "GLFogFilter.h"
#include "GLFogFilter2.h"
#include "GLFramebufferManager.h"
#include "GLImage.h"
#include "GLImageManager.h"
#include "GLImageRenderer.h"
#include "GLLensDustFilter.h"
#include "GLLensFilter.h"
#include "GLLensFlareFilter.h"
#include "GLLongSpriteRenderer.h"
#include "GLMapRenderer.h"
#include "GLMapShadowRenderer.h"
#include "GLModel.h"
#include "GLModelManager.h"
#include "GLModelRenderer.h"
#include "GLNonlinearizeFilter.h"
#include "GLOptimizedVoxelModel.h"
#include "GLProfiler.h"
#include "GLProgramAttribute.h"
#include "GLProgramManager.h"
#include "GLProgramUniform.h"
#include "GLRadiosityRenderer.h"
#include "GLRenderer.h"
#include "GLResampleBicubicFilter.h"
#include "GLSSAOFilter.h"
#include "GLSettings.h"
#include "GLShadowMapShader.h"
#include "GLSoftLitSpriteRenderer.h"
#include "GLSoftSpriteRenderer.h"
#include "GLSpriteRenderer.h"
#include "GLTemporalAAFilter.h"
#include "GLVoxelModel.h"
#include "GLWaterRenderer.h"
#include "IGLDevice.h"
#include "IGLShadowMapRenderer.h"
#include <Client/GameMap.h>
#include <Core/Bitmap.h>
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include <Core/Stopwatch.h>

namespace spades {
	namespace draw {
		// TODO: raise error for any calls after Shutdown().

		GLRenderer::GLRenderer(Handle<IGLDevice> _device)
		    : device(std::move(_device)),
		      map(NULL),
		      inited(false),
		      sceneUsedInThisFrame(false),
		      programManager(NULL),
		      imageManager(NULL),
		      modelManager(NULL),
		      mapShadowRenderer(NULL),
		      mapRenderer(NULL),
		      imageRenderer(NULL),
		      flatMapRenderer(NULL),
		      modelRenderer(NULL),
		      spriteRenderer(NULL),
		      longSpriteRenderer(NULL),
		      ambientShadowRenderer(NULL),
		      radiosityRenderer(NULL),
		      cameraBlur(NULL),
		      lensDustFilter(NULL),
		      autoExposureFilter(NULL),
		      lastColorBufferTexture(0),
		      fogDistance(128.f),
		      renderingMirror(false),
		      lastTime(0),
		      duringSceneRendering(false) {
			SPADES_MARK_FUNCTION();

			SPAssert(device);

			SPLog("GLRenderer bootstrap");

			// Report invalid settings via `SPLog`, which might be useful for diagnosis.
			settings.ValidateSettings();

			renderWidth = renderHeight = -1;

			UpdateRenderSize();

			programManager = new GLProgramManager(*device, shadowMapRenderer.get(), settings);
			imageManager = new GLImageManager(*device);
			imageRenderer = new GLImageRenderer(*this);
			profiler.reset(new GLProfiler(*this));

			smoothedFogColor = MakeVector3(-1.f, -1.f, -1.f);

			// ready for 2d draw
			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
			                  IGLDevice::One);
			device->Enable(IGLDevice::Blend, true);

			SPLog("GLRenderer started");
		}

		GLRenderer::~GLRenderer() {
			SPADES_MARK_FUNCTION();

			Shutdown();
		}

		void GLRenderer::UpdateRenderSize() {
			float renderScale = settings.r_scale;
			renderScale = std::max(0.2f, renderScale);
			if (!(renderScale < 1.0f)) { // eliminates NaN
				renderScale = 1.0f;
			}

			int screenWidth = device->ScreenWidth();
			int screenHeight = device->ScreenHeight();

			int newRenderWidth = static_cast<int>(screenWidth * renderScale);
			int newRenderHeight = static_cast<int>(screenHeight * renderScale);

			if (newRenderWidth != renderWidth || newRenderHeight != renderHeight) {
				SPLog("Render size has changed");
				renderWidth = newRenderWidth;
				renderHeight = newRenderHeight;

				fbManager.reset(
				  new GLFramebufferManager(*device, settings, renderWidth, renderHeight));
				if (waterRenderer) {
					SPLog("Creating Water Renderer");
					waterRenderer.reset(new GLWaterRenderer(*this, map));
				}
			}
		}

		void GLRenderer::Init() {
			if (modelManager != NULL) {
				// already initialized
				return;
			}

			SPLog("GLRenderer initializing for 3D rendering");
			shadowMapRenderer = GLShadowMapShader::CreateShadowMapRenderer(*this);
			modelManager = new GLModelManager(*this);
			if ((int)settings.r_softParticles >= 2)
				spriteRenderer = new GLSoftLitSpriteRenderer(*this);
			else if (settings.r_softParticles)
				spriteRenderer = new GLSoftSpriteRenderer(*this);
			else
				spriteRenderer = new GLSpriteRenderer(*this);
			longSpriteRenderer = new GLLongSpriteRenderer(*this);
			modelRenderer = new GLModelRenderer(*this);

			// preload
			SPLog("Preloading Assets");
			GLMapRenderer::PreloadShaders(*this);
			GLVoxelModel::PreloadShaders(*this);
			GLOptimizedVoxelModel::PreloadShaders(*this);
			if (settings.r_water)
				GLWaterRenderer::PreloadShaders(*this);

			if (settings.r_cameraBlur) {
				cameraBlur = new GLCameraBlurFilter(*this);
			}
			if (settings.r_temporalAA) {
				temporalAAFilter.reset(new GLTemporalAAFilter(*this));
			}

			if (settings.ShouldUseFogFilter2()) {
				fogFilter2.reset(new GLFogFilter2(*this));
			} else if (settings.r_fogShadow) {
				GLFogFilter(*this);
			}

			if (settings.r_bloom) {
				lensDustFilter = new GLLensDustFilter(*this);
			}

			if (settings.r_hdr) {
				autoExposureFilter = new GLAutoExposureFilter(*this);
			}

			if (settings.r_lens) {
				GLLensFilter(*this);
			}

			if (settings.r_lensFlare) {
				GLLensFlareFilter(*this);
			}

			if (settings.r_colorCorrection) {
				GLColorCorrectionFilter(*this);
			}

			if (settings.r_fxaa) {
				GLFXAAFilter(*this);
			}

			if (settings.r_scaleFilter.operator int() == 2) {
				GLResampleBicubicFilter(*this);
			}

			if (settings.r_depthOfField) {
				GLDepthOfFieldFilter(*this);
			}

			device->Finish();
			SPLog("GLRenderer initialized");
		}

		void GLRenderer::Shutdown() {
			// FIXME: remove itself from map's listener

			SPLog("GLRender finalizing");
			SetGameMap(nullptr);
			temporalAAFilter.reset();
			fogFilter2.reset();
			delete autoExposureFilter;
			autoExposureFilter = NULL;
			delete lensDustFilter;
			lensDustFilter = NULL;
			delete radiosityRenderer;
			radiosityRenderer = NULL;
			delete ambientShadowRenderer;
			ambientShadowRenderer = NULL;
			delete flatMapRenderer;
			flatMapRenderer = NULL;
			delete mapShadowRenderer;
			mapShadowRenderer = NULL;
			delete mapRenderer;
			mapRenderer = NULL;
			waterRenderer.reset();
			delete ambientShadowRenderer;
			ambientShadowRenderer = NULL;
			shadowMapRenderer.reset();
			delete cameraBlur;
			cameraBlur = NULL;
			delete longSpriteRenderer;
			longSpriteRenderer = NULL;
			delete modelRenderer;
			modelRenderer = NULL;
			delete spriteRenderer;
			spriteRenderer = NULL;
			delete imageRenderer;
			imageRenderer = NULL;
			delete modelManager;
			modelManager = NULL;
			delete programManager;
			programManager = NULL;
			delete imageManager;
			imageManager = NULL;
			fbManager.reset();
			profiler.reset();
			SPLog("GLRenderer finalized");
		}

		Handle<client::IImage> GLRenderer::RegisterImage(const char *filename) {
			SPADES_MARK_FUNCTION();
			return imageManager->RegisterImage(filename);
		}

		Handle<client::IModel> GLRenderer::RegisterModel(const char *filename) {
			SPADES_MARK_FUNCTION();
			return modelManager->RegisterModel(filename).Cast<client::IModel>();
		}

		void GLRenderer::ClearCache() {
			SPADES_MARK_FUNCTION();
			modelManager->ClearCache();
			imageManager->ClearCache();
		}

		Handle<client::IImage> GLRenderer::CreateImage(spades::Bitmap &bmp) {
			SPADES_MARK_FUNCTION();
			return GLImage::FromBitmap(bmp, device.GetPointerOrNull()).Cast<client::IImage>();
		}

		Handle<client::IModel> GLRenderer::CreateModel(spades::VoxelModel &model) {
			SPADES_MARK_FUNCTION();
			return Handle<GLVoxelModel>::New(&model, *this).Cast<client::IModel>();
		}

		Handle<client::IModel> GLRenderer::CreateModelOptimized(spades::VoxelModel &model) {
			SPADES_MARK_FUNCTION();
			if (settings.r_optimizedVoxelModel) {
				return Handle<GLOptimizedVoxelModel>::New(&model, *this).Cast<client::IModel>();
			} else {
				return Handle<GLVoxelModel>::New(&model, *this).Cast<client::IModel>();
			}
		}

		void GLRenderer::EnsureInitialized() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (modelManager == NULL) {
				SPRaise("Renderer is not initialized");
			}
		}

		void GLRenderer::SetGameMap(stmp::optional<client::GameMap &> newMap) {
			SPADES_MARK_FUNCTION();

			if (newMap) {
				EnsureInitialized();
			}

			client::GameMap *oldMap = map;

			SPLog("New map loaded; freeing old renderers...");
			delete radiosityRenderer;
			radiosityRenderer = NULL;
			delete mapRenderer;
			mapRenderer = NULL;
			delete flatMapRenderer;
			flatMapRenderer = NULL;
			delete mapShadowRenderer;
			mapShadowRenderer = NULL;
			waterRenderer.reset();
			delete ambientShadowRenderer;
			ambientShadowRenderer = NULL;

			if (newMap) {
				SPLog("Creating new renderers...");

				SPLog("Creating Terrain Shadow Map Renderer");
				mapShadowRenderer = new GLMapShadowRenderer(*this, newMap.get_pointer());
				SPLog("Creating TerrainRenderer");
				mapRenderer = new GLMapRenderer(newMap.get_pointer(), *this);
				SPLog("Creating Minimap Renderer");
				flatMapRenderer = new GLFlatMapRenderer(*this, *newMap);
				SPLog("Creating Water Renderer");
				waterRenderer.reset(new GLWaterRenderer(*this, newMap.get_pointer()));

				if (settings.r_radiosity) {
					SPLog("Creating Ray-traced Ambient Occlusion Renderer");
					ambientShadowRenderer = new GLAmbientShadowRenderer(*this, *newMap);
					SPLog("Creating Relective Shadow Maps Renderer");
					radiosityRenderer = new GLRadiosityRenderer(*this, newMap.get_pointer());
				} else {
					SPLog("Radiosity is disabled");
				}

				newMap->AddListener(this);
				newMap->AddRef();
				SPLog("Created");
			} else {
				SPLog("No map loaded");
			}
			this->map = newMap.get_pointer();
			if (oldMap) {
				oldMap->RemoveListener(this);
				oldMap->Release();
			}
		}

		float GLRenderer::ScreenWidth() { return device->ScreenWidth(); }

		float GLRenderer::ScreenHeight() { return device->ScreenHeight(); }

		void GLRenderer::SetFogColor(spades::Vector3 v) {
			fogColor = v;
			if (smoothedFogColor.x < 0.f)
				smoothedFogColor = fogColor;
		}

		Vector3 GLRenderer::GetFogColorForSolidPass() {
			if (settings.r_fogShadow && mapShadowRenderer) {
				return MakeVector3(0, 0, 0);
			} else {
				return GetFogColor();
			}
		}

#pragma mark - Resource Manager

		GLProgram *GLRenderer::RegisterProgram(const std::string &name) {
			return programManager->RegisterProgram(name);
		}

		GLShader *GLRenderer::RegisterShader(const std::string &name) {
			return programManager->RegisterShader(name);
		}

#pragma mark - Scene Intiializer

		void GLRenderer::BuildProjectionMatrix() {
			SPADES_MARK_FUNCTION();

			Matrix4 mat = sceneDef.ToOpenGLProjectionMatrix();

			if (settings.r_temporalAA && temporalAAFilter) {
				float jitterX = 1.0f / GetRenderWidth();
				float jitterY = 1.0f / GetRenderHeight();
				Vector2 jitter = temporalAAFilter->GetProjectionMatrixJitter();
				jitterX *= jitter.x * 1.3f;
				jitterY *= jitter.y * 1.3f;
				mat = Matrix4::Translate(jitterX, jitterY, 0.0f) * mat;
			}

			projectionMatrix = mat;
		}

		void GLRenderer::BuildView() {
			SPADES_MARK_FUNCTION();

			viewMatrix = sceneDef.ToViewMatrix();
		}

		void GLRenderer::BuildFrustrum() {
			// far/near
			frustrum[0] = Plane3::PlaneWithPointOnPlane(sceneDef.viewOrigin, sceneDef.viewAxis[2]);
			frustrum[1] = frustrum[0].Flipped();
			frustrum[0].w -= sceneDef.zNear;
			frustrum[1].w += sceneDef.zFar;

			float xCos = cosf(sceneDef.fovX * .5f);
			float xSin = sinf(sceneDef.fovX * .5f);
			float yCos = cosf(sceneDef.fovY * .5f);
			float ySin = sinf(sceneDef.fovY * .5f);

			frustrum[2] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * xSin - sceneDef.viewAxis[0] * xCos);
			frustrum[3] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * xSin + sceneDef.viewAxis[0] * xCos);
			frustrum[4] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * ySin - sceneDef.viewAxis[1] * yCos);
			frustrum[5] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * ySin + sceneDef.viewAxis[1] * yCos);
		}

		void GLRenderer::EnsureSceneStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (!duringSceneRendering) {
				SPRaise("Illegal call outside of StartScene ... EndScene");
			}
		}

		void GLRenderer::EnsureSceneNotStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (duringSceneRendering) {
				SPRaise("Illegal call between StartScene ... EndScene");
			}
		}

		void GLRenderer::StartScene(const client::SceneDefinition &def) {
			SPADES_MARK_FUNCTION();

			EnsureInitialized();

			if (def.radialBlur < 0.f || def.radialBlur > 1.f)
				SPRaise("Invalid value of radialBlur.");
			sceneDef = def;

			sceneUsedInThisFrame = true;
			duringSceneRendering = true;

			profiler->BeginFrame();

			// clear scene objects
			debugLines.clear();
			spriteRenderer->Clear();
			longSpriteRenderer->Clear();
			modelRenderer->Clear();
			lights.clear();

			device->DepthMask(true);

			BuildProjectionMatrix();
			BuildView();
			BuildFrustrum();

			projectionViewMatrix = projectionMatrix * viewMatrix;

			if (settings.r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, true);
		}

#pragma mark - Add Scene Objects

		void GLRenderer::RenderModel(client::IModel &model, const client::ModelRenderParam &param) {
			SPADES_MARK_FUNCTION();

			GLModel &glModel = dynamic_cast<GLModel &>(model);

			EnsureInitialized();
			EnsureSceneStarted();

			// TODO: early frustrum cull?

			modelRenderer->AddModel(&glModel, param);
		}

		void GLRenderer::AddLight(const client::DynamicLightParam &light) {
			if (!settings.r_dlights)
				return;
			if (!SphereFrustrumCull(light.origin, light.radius))
				return;
			EnsureInitialized();
			EnsureSceneStarted();
			lights.push_back(GLDynamicLight(light));
		}

		void GLRenderer::AddDebugLine(spades::Vector3 a, spades::Vector3 b, spades::Vector4 color) {
			DebugLine line = {a, b, color};
			EnsureInitialized();
			EnsureSceneStarted();
			debugLines.push_back(line);
		}

		void GLRenderer::AddSprite(client::IImage &img, spades::Vector3 center, float radius,
		                           float rotation) {
			SPADES_MARK_FUNCTION_DEBUG();
			GLImage &glImage = dynamic_cast<GLImage &>(img);

			if (!SphereFrustrumCull(center, radius * 1.5f))
				return;

			EnsureInitialized();
			EnsureSceneStarted();

			spriteRenderer->Add(&glImage, center, radius, rotation, drawColorAlphaPremultiplied);
		}

		void GLRenderer::AddLongSprite(client::IImage &img, spades::Vector3 p1, spades::Vector3 p2,
		                               float radius) {
			SPADES_MARK_FUNCTION_DEBUG();
			GLImage &glImage = dynamic_cast<GLImage &>(img);

			EnsureInitialized();
			EnsureSceneStarted();

			longSpriteRenderer->Add(&glImage, p1, p2, radius, drawColorAlphaPremultiplied);
		}

#pragma mark - Scene Finalizer

		struct DebugLineVertex {
			float x, y, z;
			float r, g, b, a;
			static DebugLineVertex Create(Vector3 v, Vector4 col) {
				DebugLineVertex vv = {v.x, v.y, v.z, col.x, col.y, col.z, col.w};
				return vv;
			}
		};
		void GLRenderer::RenderDebugLines() {
			SPADES_MARK_FUNCTION();
			if (debugLines.empty())
				return;

			// build vertices
			std::vector<DebugLineVertex> vertices;
			vertices.resize(debugLines.size() * 2);

			for (size_t i = 0, j = 0; i < debugLines.size(); i++) {
				const DebugLine &line = debugLines[i];
				vertices[j++] = DebugLineVertex::Create(line.v1, line.color);
				vertices[j++] = DebugLineVertex::Create(line.v2, line.color);
			}

			GLProgram *program = RegisterProgram("Shaders/Basic.program");
			program->Use();

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");

			positionAttribute(program);
			colorAttribute(program);

			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::FloatType, false,
			                            sizeof(DebugLineVertex), vertices.data());
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::FloatType, false,
			                            sizeof(DebugLineVertex),
			                            (const char *)vertices.data() + sizeof(float) * 3);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);

			static GLProgramUniform projectionViewMatrix("projectionViewMatrix");
			projectionViewMatrix(program);
			projectionViewMatrix.SetValue(GetProjectionViewMatrix());

			device->DrawArrays(IGLDevice::Lines, 0, static_cast<IGLDevice::Sizei>(vertices.size()));

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);
		}

		void GLRenderer::RenderObjects() {

			// draw opaque objects, and do dynamic lighting

			device->Enable(IGLDevice::DepthTest, true);
			device->Enable(IGLDevice::Texture2D, true);
			device->Enable(IGLDevice::Blend, false);

			bool needsDepthPrepass = settings.r_depthPrepass || settings.r_ssao;
			bool needsFullDepthPrepass = settings.r_ssao;

			GLFramebufferManager::BufferHandle ssaoBuffer;

			if (needsDepthPrepass) {
				{
					GLProfiler::Context p(*profiler, "Depth-only Prepass");
					device->DepthFunc(IGLDevice::Less);
					if (!sceneDef.skipWorld && mapRenderer) {
						mapRenderer->Prerender();
					}
					if (needsFullDepthPrepass) {
						modelRenderer->Prerender(false);
					}
				}

				if (settings.r_ssao) {
					{
						GLProfiler::Context p(*profiler, "Screen Space Ambient Occlusion");
						device->DepthMask(false);
						device->Enable(IGLDevice::DepthTest, false);
						device->Enable(IGLDevice::CullFace, false);

						ssaoBuffer = GLSSAOFilter(*this).Filter();
						ssaoBufferTexture = ssaoBuffer.GetTexture();

						device->BindTexture(IGLDevice::Texture2D, ssaoBufferTexture);
						device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
						                     IGLDevice::Nearest);
						device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
						                     IGLDevice::Nearest);

						device->Enable(IGLDevice::CullFace, true);
					}
					GetFramebufferManager()->PrepareSceneRendering();
				}
			}
			{
				GLProfiler::Context p(*profiler, "Sunlight Pass");

				device->DepthFunc(IGLDevice::LessOrEqual);
				if (!sceneDef.skipWorld && mapRenderer) {
					mapRenderer->RenderSunlightPass();
				}
				modelRenderer->RenderSunlightPass(false);
			}
			if (settings.r_ssao) {
				device->BindTexture(IGLDevice::Texture2D, ssaoBufferTexture);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                     IGLDevice::Linear);
				ssaoBuffer.Release();
			}

			{
				GLProfiler::Context p(*profiler, "Dynamic Light Pass [%d light(s)]",
				                      (int)lights.size());

				device->Enable(IGLDevice::Blend, true);
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Equal);
				device->BlendFunc(IGLDevice::SrcAlpha, IGLDevice::One, IGLDevice::Zero,
				                  IGLDevice::One);

				if (!sceneDef.skipWorld && mapRenderer) {
					mapRenderer->RenderDynamicLightPass(lights);
				}
				modelRenderer->RenderDynamicLightPass(lights);
			}

			{
				GLProfiler::Context p(*profiler, "Debug Line");
				device->Enable(IGLDevice::Blend, false);
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Less);
				RenderDebugLines();
			}
		}

		void GLRenderer::RenderGhosts() {
			// Run a depth-only pass so that for each pixel, objects are drawn
			// only once
			{
				GLProfiler::Context p(*profiler, "Depth-only Prepass");
				device->DepthFunc(IGLDevice::Less);
				modelRenderer->Prerender(true);
			}

			// Run a color pass
			{
				GLProfiler::Context p(*profiler, "Ghost Pass");

				device->Enable(IGLDevice::Blend, true);
				device->DepthMask(false);
				device->BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
				                  IGLDevice::One);
				device->DepthFunc(IGLDevice::Equal);
				modelRenderer->RenderSunlightPass(true);
			}
			device->DepthMask(true);
			device->DepthFunc(IGLDevice::Less);
			device->Enable(IGLDevice::Blend, false);
			device->Enable(IGLDevice::CullFace, false);
		}

		void GLRenderer::EndScene() {
			SPADES_MARK_FUNCTION();

			EnsureInitialized();
			EnsureSceneStarted();

			GLProfiler::Context pRoot(*profiler, "EndScene");

			float dt = (float)(sceneDef.time - lastTime) / 1000.f;
			if (dt > .1f)
				dt = .1f;
			if (dt < 0.f)
				dt = 0.f;

			duringSceneRendering = false;
			renderingMirror = false;

			{
				GLProfiler::Context p(*profiler, "Upload Dynamic Data");
				if (mapShadowRenderer) {
					mapShadowRenderer->Update();
				}
				if (ambientShadowRenderer) {
					ambientShadowRenderer->Update();
				}
				if (radiosityRenderer) {
					radiosityRenderer->Update();
				}
				if (mapRenderer) {
					mapRenderer->Realize();
				}
			}

			if (settings.r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, false);

			// build shadowmap
			{
				GLProfiler::Context p(*profiler, "Shadow Map Pass");
				device->Enable(IGLDevice::DepthTest, true);
				device->DepthFunc(IGLDevice::Less);
				if (shadowMapRenderer)
					shadowMapRenderer->Render();
			}

			fbManager->PrepareSceneRendering();

			if (settings.r_srgb)
				device->Enable(IGLDevice::FramebufferSRGB, true);

			Vector3 bgCol;

			device->ClearDepth(1.f);
			device->DepthRange(0.f, 1.f);

			if ((int)settings.r_water >= 2) {
				// for Water 2 (r_water >= 2), we need to render
				// reflection
				try {

					IGLDevice::UInteger occQu =
					  waterRenderer ? waterRenderer->GetOcclusionQuery() : 0;

					device->FrontFace(IGLDevice::CCW);
					renderingMirror = true;

					// save normal matrices
					Matrix4 view;
					view = viewMatrix * Matrix4::Translate(0, 0, 63);
					view = view * Matrix4::Scale(1, 1, -1);
					view = view * Matrix4::Translate(0, 0, -63);

					std::swap(view, viewMatrix);
					projectionViewMatrix = projectionMatrix * viewMatrix;

					if (occQu) {
						fbManager->ClearMirrorTexture(GetFogColor());
						device->BeginConditionalRender(occQu, IGLDevice::QueryWait);
					}

					bgCol = GetFogColorForSolidPass();
					if (settings.r_hdr) {
						bgCol *= bgCol;
					} // linearlize
					{
						GLProfiler::Context p(*profiler, "Clear (Mirrored Scene)");
						device->ClearColor(bgCol.x, bgCol.y, bgCol.z, 1.f);
						device->Clear(
						  (IGLDevice::Enum)(IGLDevice::ColorBufferBit | IGLDevice::DepthBufferBit));
					}

					// render scene
					GLProfiler::Context p(*profiler, "Mirrored Objects");
					RenderObjects();

					// restore matrices
					std::swap(view, viewMatrix);
					projectionViewMatrix = projectionMatrix * viewMatrix;

					if (settings.r_fogShadow && mapShadowRenderer) {
						GLProfiler::Context p(*profiler, "Volumetric Fog");

						GLFramebufferManager::BufferHandle handle;

						handle = fbManager->StartPostProcessing();
						if (settings.ShouldUseFogFilter2()) {
							if (!fogFilter2) {
								fogFilter2.reset(new GLFogFilter2(*this));
							}
							handle = fogFilter2->Filter(handle);
						} else {
							handle = GLFogFilter(*this).Filter(handle);
						}
						fbManager->CopyToMirrorTexture(handle.GetFramebuffer());
					} else {
						fbManager->CopyToMirrorTexture();
					}

					if (occQu) {
						device->EndConditionalRender();
					}

					renderingMirror = false;
				} catch (...) {
					renderingMirror = false;
					throw;
				}
			}

			bgCol = GetFogColorForSolidPass();
			if (settings.r_hdr) {
				bgCol *= bgCol;
			} // linearlize
			{
				GLProfiler::Context p(*profiler, "Clear");
				device->ClearColor(bgCol.x, bgCol.y, bgCol.z, 1.f);
				device->Clear(
				  (IGLDevice::Enum)(IGLDevice::ColorBufferBit | IGLDevice::DepthBufferBit));
			}

			device->FrontFace(IGLDevice::CW);

			{
				GLProfiler::Context p(*profiler, "Non-mirrored Objects");
				RenderObjects();
			}

			device->Enable(IGLDevice::CullFace, false);
			if (settings.r_water && waterRenderer) {
				GLProfiler::Context p(*profiler, "Water");
				waterRenderer->Update(dt);
				waterRenderer->Render();
			}

			{
				GLProfiler::Context p(*profiler, "Ghosts");
				RenderGhosts();
			}

			device->Enable(IGLDevice::Blend, true);

			device->DepthMask(false);
			if (!settings.r_softParticles) { // softparticle is a part of postprocess
				GLProfiler::Context p(*profiler, "Particles");
				device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
				                  IGLDevice::One);
				spriteRenderer->Render();
			}

			{
				GLProfiler::Context p(*profiler, "Long Particles");
				device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
				                  IGLDevice::One);
				longSpriteRenderer->Render();
			}

			device->Enable(IGLDevice::DepthTest, false);

			GLFramebufferManager::BufferHandle handle;
			{
				GLProfiler::Context p(*profiler, "Post-process");

				// now process the non-multisampled buffer.
				// depth buffer is also can be read
				{
					GLProfiler::Context p(*profiler, "Preparation");
					handle = fbManager->StartPostProcessing();
				}
				if (settings.r_fogShadow && mapShadowRenderer) {
					GLProfiler::Context p(*profiler, "Volumetric Fog");
					if (settings.ShouldUseFogFilter2()) {
						if (!fogFilter2) {
							fogFilter2.reset(new GLFogFilter2(*this));
						}
						handle = fogFilter2->Filter(handle);
					} else {
						handle = GLFogFilter(*this).Filter(handle);
					}
				}
				device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());

				if (settings.r_softParticles) { // softparticle is a part of postprocess
					GLProfiler::Context p(*profiler, "Soft Particle");
					device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
					                  IGLDevice::One);
					spriteRenderer->Render();
				}

				device->BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
				                  IGLDevice::One);

				if (settings.r_depthOfField &&
				    (sceneDef.depthOfFieldFocalLength > 0.f || sceneDef.blurVignette > 0.f)) {
					GLProfiler::Context p(*profiler, "Depth of Field");
					handle = GLDepthOfFieldFilter(*this).Filter(
					  handle, sceneDef.depthOfFieldFocalLength, sceneDef.blurVignette,
					  sceneDef.globalBlur, sceneDef.depthOfFieldNearBlurStrength,
					  sceneDef.depthOfFieldFarBlurStrength);
				}

				if (settings.r_cameraBlur && !sceneDef.denyCameraBlur) {
					if (!cameraBlur) {
						cameraBlur = new GLCameraBlurFilter(*this);
					}

					GLProfiler::Context p(*profiler, "Camera Blur");
					// FIXME: better (correctly constructed) radial blur algorithm
					handle = cameraBlur->Filter(
					  handle, std::min(settings.r_cameraBlur * 0.2f, 1.0f), sceneDef.radialBlur);
				}

				if (settings.r_temporalAA) {
					if (!temporalAAFilter) {
						temporalAAFilter.reset(new GLTemporalAAFilter(*this));
					}
					GLProfiler::Context p(*profiler, "Temporal AA");
					handle = temporalAAFilter->Filter(handle, settings.r_fxaa);
				}

				if (settings.r_bloom) {
					GLProfiler::Context p(*profiler, "Bloom");
					handle = lensDustFilter->Filter(handle);
				}

				// Do r_fxaa before lens filter so that color aberration looks nice.
				if (settings.r_fxaa) {
					GLProfiler::Context p(*profiler, "FXAA");
					handle = GLFXAAFilter(*this).Filter(handle);
				}

				if (settings.r_lens) {
					GLProfiler::Context p(*profiler, "Lens Filter");
					handle = GLLensFilter(*this).Filter(handle);
				}

				if (settings.r_lensFlare) {
					GLProfiler::Context p(*profiler, "Lens Flare");
					device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());
					GLLensFlareFilter(*this).Draw();
				}

				if (settings.r_lensFlare && settings.r_lensFlareDynamic) {
					GLProfiler::Context p(*profiler, "Dynamic Light Lens Flare");
					GLLensFlareFilter lensFlareRenderer(*this);
					device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());
					for (size_t i = 0; i < lights.size(); i++) {
						const GLDynamicLight &dl = lights[i];
						const client::DynamicLightParam &prm = dl.GetParam();
						if (!prm.useLensFlare)
							continue;
						Vector3 color = prm.color * 0.6f;
						{
							// distance attenuation
							float rad = (prm.origin - sceneDef.viewOrigin).GetPoweredLength();
							rad /= prm.radius * prm.radius * 18.f;
							if (rad > 1.f)
								continue;
							color *= 1.f - rad;
						}

						if (prm.type == client::DynamicLightTypeSpotlight) {
							// spotlight
							Vector3 diff = (sceneDef.viewOrigin - prm.origin).Normalize();
							Vector3 lightdir = prm.spotAxis[2];
							lightdir = lightdir.Normalize();
							float cosVal = Vector3::Dot(diff, lightdir);
							float minCosVal = cosf(prm.spotAngle * 0.5f);
							if (cosVal < minCosVal) {
								// out of range
								continue;
							}
							color *= (cosVal - minCosVal) / (1.f - minCosVal);
						}

						// view cull
						if (Vector3::Dot(sceneDef.viewAxis[2], (prm.origin - sceneDef.viewOrigin)) <
						    0.f) {
							continue;
						}

						lensFlareRenderer.Draw(prm.origin - sceneDef.viewOrigin, true, color,
						                       false);
					}
				}

				// FIXME: these passes should be combined for lower VRAM bandwidth usage

				if (settings.r_hdr) {
					GLProfiler::Context p(*profiler, "Auto Exposure");
					handle = autoExposureFilter->Filter(handle, dt);
				}

				if (settings.r_hdr) {
					GLProfiler::Context p(*profiler, "Gamma Correction");
					handle = GLNonlinearlizeFilter(*this).Filter(handle);
				}

				if (settings.r_colorCorrection) {
					GLProfiler::Context p(*profiler, "Color Correction");
					Vector3 tint = smoothedFogColor + MakeVector3(1.f, 1.f, 1.f) * 0.5f;
					tint = MakeVector3(1.f, 1.f, 1.f) / tint;
					tint = Mix(tint, MakeVector3(1.f, 1.f, 1.f), 0.2f);
					tint *= 1.f / std::min(std::min(tint.x, tint.y), tint.z);

					float fogLuminance = (fogColor.x + fogColor.y + fogColor.z) * (1.0f / 3.0f);

					if (settings.ShouldUseFogFilter2()) {
						// `GLFogFilter2` adds a GI factor, so the fog receives some light
						// even if the fog color is set to dark.
						fogLuminance = fogLuminance * 0.9 + 0.2;
					}

					float exposure = powf(2.f, (float)settings.r_exposureValue * 0.5f);
					handle =
					  GLColorCorrectionFilter(*this).Filter(handle, tint * exposure, fogLuminance);

					// update smoothed fog color
					smoothedFogColor = Mix(smoothedFogColor, fogColor, 0.002f);
				}
			}

			// Resample the rendered image using a non-trivial filter if such
			// a filter is selected.
			int scaleFilter = settings.r_scaleFilter;
			bool scalingMayBeNeeded = GetRenderWidth() != device->ScreenWidth() ||
			                          GetRenderHeight() != device->ScreenHeight();
			if (scaleFilter == 0) {
				// Nearest neighbor - trivial
			} else if (scaleFilter == 1) {
				// Bi-linear - trivial
			} else if (scaleFilter == 2) {
				// Bi-cubic - non-trivial
				handle = GLResampleBicubicFilter{*this}.Filter(handle, device->ScreenWidth(),
				                                               device->ScreenHeight());
				scaleFilter = 0;
			} else {
				// I don't know this filter.
				scaleFilter = 1;
			}

			if (settings.r_srgb && settings.r_srgb2D) {
				// in gamma corrected mode,
				// 2d drawings are done on gamma-corrected FB
				// see also: FrameDone
				// TODO: Handle the case where `scaleFilter == 0`
				lastColorBufferTexture = handle.GetTexture();
				device->BindFramebuffer(IGLDevice::Framebuffer, handle.GetFramebuffer());
				device->Enable(IGLDevice::FramebufferSRGB, true);

			} else {
				// copy buffer to WM given framebuffer
				if (settings.r_srgb)
					device->Enable(IGLDevice::FramebufferSRGB, false);

				GLProfiler::Context p(*profiler, "Copying to WM-given Framebuffer");

				if (scaleFilter == 0) {
					// Temporarily change the textute filter to NN
					device->BindTexture(IGLDevice::Texture2D, handle.GetTexture());
					device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
					                     IGLDevice::Nearest);
					device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
					                     IGLDevice::Nearest);
				}

				device->BindFramebuffer(IGLDevice::Framebuffer, 0);
				device->Enable(IGLDevice::Blend, false);
				device->Viewport(0, 0, device->ScreenWidth(), device->ScreenHeight());
				Handle<GLImage> image(new GLImage(handle.GetTexture(), device.GetPointerOrNull(),
				                                  handle.GetWidth(), handle.GetHeight(), false),
				                      false);
				SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1));
				DrawImage(*image, AABB2(0, device->ScreenHeight(), device->ScreenWidth(),
				                        -device->ScreenHeight()));
				imageRenderer->Flush();

				if (scaleFilter == 0) {
					// Reset the texture filter
					device->BindTexture(IGLDevice::Texture2D, handle.GetTexture());
					device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
					                     IGLDevice::Linear);
					device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
					                     IGLDevice::Linear);
				}
			}

			handle.Release();
			fbManager->MakeSureAllBuffersReleased();

			// model renderer must be cleared because
			// GLModelRenderer::Clear accesses added model and
			// some models might be deleted before the next frame
			modelRenderer->Clear();

			// prepare for 2d drawing
			Prepare2DRendering(true);
		}

		//#pragma mark - 2D Drawings

		void GLRenderer::MultiplyScreenColor(spades::Vector3 color) {
			SPADES_MARK_FUNCTION();
			void EnsureSceneNotStarted();
			imageRenderer->Flush();

			device->BlendFunc(IGLDevice::Zero, IGLDevice::SrcColor, IGLDevice::Zero,
			                  IGLDevice::One);

			Vector4 col = {color.x, color.y, color.z, 1};

			// build vertices
			DebugLineVertex vertices[4];

			vertices[0] = DebugLineVertex::Create(MakeVector3(-1, -1, 0), col);
			vertices[1] = DebugLineVertex::Create(MakeVector3(1, -1, 0), col);
			vertices[2] = DebugLineVertex::Create(MakeVector3(1, 1, 0), col);
			vertices[3] = DebugLineVertex::Create(MakeVector3(-1, 1, 0), col);

			GLProgram *program = RegisterProgram("Shaders/Basic.program");
			program->Use();

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");

			positionAttribute(program);
			colorAttribute(program);

			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::FloatType, false,
			                            sizeof(DebugLineVertex), vertices);
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::FloatType, false,
			                            sizeof(DebugLineVertex),
			                            (const char *)vertices + sizeof(float) * 3);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);

			static GLProgramUniform projectionViewMatrix("projectionViewMatrix");
			projectionViewMatrix(program);
			projectionViewMatrix.SetValue(Matrix4::Identity());

			device->DrawArrays(IGLDevice::TriangleFan, 0, 4);

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);

			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
			                  IGLDevice::One);
		}

		void GLRenderer::DrawImage(stmp::optional<client::IImage &> image,
		                           const spades::Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();

			if (image == nullptr) {
				SPRaise("Size must be specified when null image is provided");
			}

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, image->GetWidth(), image->GetHeight()),
			          AABB2(0, 0, image->GetWidth(), image->GetHeight()));
		}

		void GLRenderer::DrawImage(stmp::optional<client::IImage &> image,
		                           const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, outRect,
			          AABB2(0, 0, image ? image->GetWidth() : 0, image ? image->GetHeight() : 0));
		}

		void GLRenderer::DrawImage(stmp::optional<client::IImage &> image,
		                           const spades::Vector2 &outTopLeft, const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, inRect.GetWidth(), inRect.GetHeight()),
			          inRect);
		}

		void GLRenderer::DrawImage(stmp::optional<client::IImage &> image,
		                           const spades::AABB2 &outRect, const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()), inRect);
		}

		void GLRenderer::DrawImage(stmp::optional<client::IImage &> image,
		                           const spades::Vector2 &outTopLeft,
		                           const spades::Vector2 &outTopRight,
		                           const spades::Vector2 &outBottomLeft,
		                           const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			void EnsureSceneNotStarted();

			// d = a + (b - a) + (c - a)
			//   = b + c - a
			Vector2 outBottomRight = outTopRight + outBottomLeft - outTopLeft;
			GLImage *img = dynamic_cast<GLImage *>(image.get_pointer());
			if (!img) {
				if (!image) {
					img = imageManager->GetWhiteImage();
				} else {
					// invalid type: not GLImage.
					SPInvalidArgument("image");
				}
			}

			imageRenderer->SetImage(img);

			Vector4 col = drawColorAlphaPremultiplied;
			if (legacyColorPremultiply) {
				// in legacy mode, image color is
				// non alpha-premultiplied.
				col.x *= col.w;
				col.y *= col.w;
				col.z *= col.w;
			}

			imageRenderer->Add(outTopLeft.x, outTopLeft.y, outTopRight.x, outTopRight.y,
			                   outBottomRight.x, outBottomRight.y, outBottomLeft.x, outBottomLeft.y,
			                   inRect.GetMinX(), inRect.GetMinY(), inRect.GetMaxX(),
			                   inRect.GetMinY(), inRect.GetMaxX(), inRect.GetMaxY(),
			                   inRect.GetMinX(), inRect.GetMaxY(), col.x, col.y, col.z, col.w);
		}

		void GLRenderer::DrawFlatGameMap(const spades::AABB2 &outRect,
		                                 const spades::AABB2 &inRect) {
			void EnsureSceneNotStarted();
			if (flatMapRenderer)
				flatMapRenderer->Draw(outRect, inRect);
		}

		void GLRenderer::SetColor(spades::Vector4 col) {
			drawColorAlphaPremultiplied = col;
			legacyColorPremultiply = true;
		}

		void GLRenderer::SetColorAlphaPremultiplied(spades::Vector4 col) {
			legacyColorPremultiply = false;
			drawColorAlphaPremultiplied = col;
		}

		void GLRenderer::FrameDone() {
			SPADES_MARK_FUNCTION();

			EnsureSceneNotStarted();

			imageRenderer->Flush();

			if (settings.r_debugTimingOutputScreen && settings.r_debugTiming) {
				GLProfiler::Context p(*profiler, "Draw GLProfiler Results");
				profiler->DrawResult();
				imageRenderer->Flush();
			}

			if (settings.r_srgb && settings.r_srgb2D && sceneUsedInThisFrame) {
				// copy buffer to WM given framebuffer
				int w = device->ScreenWidth();
				int h = device->ScreenHeight();

				device->Enable(IGLDevice::FramebufferSRGB, false);

				GLProfiler::Context p(*profiler, "Copying to WM-given Framebuffer");

				device->BindFramebuffer(IGLDevice::Framebuffer, 0);
				device->Enable(IGLDevice::Blend, false);
				device->Viewport(0, 0, w, h);

				auto image = Handle<GLImage>::New(lastColorBufferTexture, device.GetPointerOrNull(),
				                                  w, h, false);
				SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1));
				DrawImage(*image, AABB2(0, h, w, -h));
				imageRenderer->Flush(); // must flush now because handle is released soon
			}

			lastTime = sceneDef.time;

			// ready for 2d draw of next frame
			Prepare2DRendering(true);

			profiler->EndFrame();

			++frameNumber;
		}

		void GLRenderer::Flip() {
			SPADES_MARK_FUNCTION();

			EnsureSceneNotStarted();

			device->Swap();

			UpdateRenderSize();
		}

		void GLRenderer::Prepare2DRendering(bool reset) {
			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha, IGLDevice::Zero,
			                  IGLDevice::One);
			device->Enable(IGLDevice::Blend, true);
			device->BindFramebuffer(IGLDevice::Framebuffer, 0);
			device->Viewport(0, 0, device->ScreenWidth(), device->ScreenHeight());
		}

		Handle<Bitmap> GLRenderer::ReadBitmap() {
			SPADES_MARK_FUNCTION();
			EnsureSceneNotStarted();
			auto bmp = Handle<Bitmap>::New(device->ScreenWidth(), device->ScreenHeight());
			device->ReadPixels(0, 0, device->ScreenWidth(), device->ScreenHeight(), IGLDevice::RGBA,
			                   IGLDevice::UnsignedByte, bmp->GetPixels());
			return bmp;
		}

		void GLRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			if (mapRenderer)
				mapRenderer->GameMapChanged(x, y, z, map);
			if (flatMapRenderer)
				flatMapRenderer->GameMapChanged(x, y, z, *map);
			if (mapShadowRenderer)
				mapShadowRenderer->GameMapChanged(x, y, z, map);
			if (waterRenderer)
				waterRenderer->GameMapChanged(x, y, z, map);
			if (ambientShadowRenderer)
				ambientShadowRenderer->GameMapChanged(x, y, z, map);
		}

		bool GLRenderer::BoxFrustrumCull(const AABB3 &box) {
			if (IsRenderingMirror()) {
				// reflect
				AABB3 bx = box;
				std::swap(bx.min.z, bx.max.z);
				bx.min.z = 63.f * 2.f - bx.min.z;
				bx.max.z = 63.f * 2.f - bx.max.z;
				return PlaneCullTest(frustrum[0], bx) && PlaneCullTest(frustrum[1], bx) &&
				       PlaneCullTest(frustrum[2], bx) && PlaneCullTest(frustrum[3], bx) &&
				       PlaneCullTest(frustrum[4], bx) && PlaneCullTest(frustrum[5], bx);
			}
			return PlaneCullTest(frustrum[0], box) && PlaneCullTest(frustrum[1], box) &&
			       PlaneCullTest(frustrum[2], box) && PlaneCullTest(frustrum[3], box) &&
			       PlaneCullTest(frustrum[4], box) && PlaneCullTest(frustrum[5], box);
		}
		bool GLRenderer::SphereFrustrumCull(const Vector3 &center, float radius) {
			if (IsRenderingMirror()) {
				// reflect
				Vector3 vx = center;
				vx.z = 63.f * 2.f - vx.z;
				for (int i = 0; i < 6; i++) {
					if (frustrum[i].GetDistanceTo(vx) < -radius)
						return false;
				}
				return true;
			}
			for (int i = 0; i < 6; i++) {
				if (frustrum[i].GetDistanceTo(center) < -radius)
					return false;
			}
			return true;
		}
	} // namespace draw
} // namespace spades
