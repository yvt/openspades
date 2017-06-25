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

#include <map>
#include <memory>

#include "GLCameraBlurFilter.h"
#include "GLDynamicLight.h"
#include "GLSettings.h"
#include <Client/IGameMapListener.h>
#include <Client/IRenderer.h>
#include <Client/SceneDefinition.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {

		class IGLDevice;
		class GLShader;
		class GLProgram;
		class GLProgramManager;
		class GLImageManager;
		class GLMapRenderer;
		class GLModelManager;
		class GLImageRenderer;
		class GLFlatMapRenderer;
		class IGLSpriteRenderer;
		class GLLongSpriteRenderer;
		class GLFramebufferManager;
		class GLMapShadowRenderer;
		class GLModelRenderer;
		class IGLShadowMapRenderer;
		class GLWaterRenderer;
		class GLAmbientShadowRenderer;
		class GLRadiosityRenderer;
		class GLLensDustFilter;
		class GLSoftLitSpriteRenderer;
		class GLAutoExposureFilter;
		class GLTemporalAAFilter;
		class GLProfiler;

		class GLRenderer : public client::IRenderer, public client::IGameMapListener {
			friend class GLShadowShader;
			friend class IGLShadowMapRenderer;
			friend class GLRadiosityRenderer;
			friend class GLSoftLitSpriteRenderer;

			struct DebugLine {
				Vector3 v1, v2;
				Vector4 color;
			};

			Handle<IGLDevice> device;
			std::unique_ptr<GLFramebufferManager> fbManager;
			client::GameMap *map;
			GLSettings settings;

			int renderWidth;
			int renderHeight;

			std::unique_ptr<GLProfiler> profiler;

			bool inited;
			bool sceneUsedInThisFrame;

			client::SceneDefinition sceneDef;
			Plane3 frustrum[6];
			AABB2 scissorRect;

			std::vector<DebugLine> debugLines;
			std::vector<GLDynamicLight> lights;

			GLProgramManager *programManager;
			GLImageManager *imageManager;
			GLModelManager *modelManager;

			// TODO: replace these with `unique_ptr`s
			IGLShadowMapRenderer *shadowMapRenderer;
			GLMapShadowRenderer *mapShadowRenderer;
			GLMapRenderer *mapRenderer;
			GLImageRenderer *imageRenderer;
			GLFlatMapRenderer *flatMapRenderer;
			GLModelRenderer *modelRenderer;
			IGLSpriteRenderer *spriteRenderer;
			GLLongSpriteRenderer *longSpriteRenderer;
			std::unique_ptr<GLWaterRenderer> waterRenderer;
			GLAmbientShadowRenderer *ambientShadowRenderer;
			GLRadiosityRenderer *radiosityRenderer;

			GLCameraBlurFilter *cameraBlur;
			GLLensDustFilter *lensDustFilter;
			GLAutoExposureFilter *autoExposureFilter;
			std::unique_ptr<GLTemporalAAFilter> temporalAAFilter;

			// used when r_ssao = 1. only valid while rendering objects
			IGLDevice::UInteger ssaoBufferTexture;

			float fogDistance;
			Vector3 fogColor;

			// used for color correction
			Vector3 smoothedFogColor;

			Matrix4 projectionMatrix;
			Matrix4 viewMatrix;
			Matrix4 projectionViewMatrix;
			bool renderingMirror;

			Vector4 drawColorAlphaPremultiplied;
			bool legacyColorPremultiply;

			unsigned int lastTime;

			bool duringSceneRendering;

			void BuildProjectionMatrix();
			void BuildView();
			void BuildFrustrum();

			void RenderDebugLines();

			void RenderObjects();

			void EnsureInitialized();
			void EnsureSceneStarted();
			void EnsureSceneNotStarted();

			void UpdateRenderSize();

			void Prepare2DRendering(bool reset = false);

		protected:
			~GLRenderer();

			void InitLowLevel() override;
			void ShutdownLowLevel() override;

			void SetScissorLowLevel(const AABB2 &) override;

			void DrawImageLowLevel(client::IImage *, const Vector2 &outTopLeft,
			                       const Vector2 &outTopRight, const Vector2 &outBottomLeft,
			                       const AABB2 &inRect) override;

		public:
			GLRenderer(IGLDevice *glDevice);

			client::IImage *RegisterImage(const char *filename) override;
			client::IModel *RegisterModel(const char *filename) override;

			client::IImage *CreateImage(Bitmap *) override;
			client::IModel *CreateModel(VoxelModel *) override;
			client::IModel *CreateModelOptimized(VoxelModel *);

			GLProgram *RegisterProgram(const std::string &name);
			GLShader *RegisterShader(const std::string &name);

			void SetGameMap(client::GameMap *) override;
			void SetFogColor(Vector3 v) override;
			void SetFogDistance(float f) override { fogDistance = f; }

			Vector3 GetFogColor() { return fogColor; }
			float GetFogDistance() { return fogDistance; }
			Vector3 GetFogColorForSolidPass();

			void StartScene(const client::SceneDefinition &) override;

			void RenderModel(client::IModel *, const client::ModelRenderParam &) override;

			void AddLight(const client::DynamicLightParam &light) override;

			void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) override;

			void AddSprite(client::IImage *, Vector3 center, float radius, float rotation) override;
			void AddLongSprite(client::IImage *, Vector3 p1, Vector3 p2, float radius) override;

			void EndScene() override;

			void MultiplyScreenColor(Vector3) override;

			void SetColor(Vector4) override;
			void SetColorAlphaPremultiplied(Vector4) override;

			void Blur() override;

			void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect) override;

			void FrameDone() override;
			void Flip() override;
			Bitmap *ReadBitmap() override;

			float ScreenWidth() override;
			float ScreenHeight() override;

			int GetRenderWidth() const { return renderWidth; }
			int GetRenderHeight() const { return renderHeight; }

			GLSettings &GetSettings() { return settings; }
			IGLDevice *GetGLDevice() { return device; }
			GLProfiler &GetGLProfiler() { return *profiler; }
			GLFramebufferManager *GetFramebufferManager() { return fbManager.get(); }
			IGLShadowMapRenderer *GetShadowMapRenderer() { return shadowMapRenderer; }
			GLAmbientShadowRenderer *GetAmbientShadowRenderer() { return ambientShadowRenderer; }
			GLMapShadowRenderer *GetMapShadowRenderer() { return mapShadowRenderer; }
			GLRadiosityRenderer *GetRadiosityRenderer() { return radiosityRenderer; }
			GLModelRenderer *GetModelRenderer() { return modelRenderer; }

			const Matrix4 &GetProjectionMatrix() const { return projectionMatrix; }
			const Matrix4 &GetProjectionViewMatrix() const { return projectionViewMatrix; }
			const Matrix4 &GetViewMatrix() const { return viewMatrix; }

			bool IsRenderingMirror() const { return renderingMirror; }

			void GameMapChanged(int x, int y, int z, client::GameMap *) override;

			const client::SceneDefinition &GetSceneDef() const { return sceneDef; }

			bool BoxFrustrumCull(const AABB3 &);
			bool SphereFrustrumCull(const Vector3 &center, float radius);
		};
	}
}
