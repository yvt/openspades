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

#include "../Client/IGameMapListener.h"
#include "../Client/IRenderer.h"
#include "../Client/SceneDefinition.h"
#include "../Core/Math.h"
#include "GLCameraBlurFilter.h"
#include "GLDynamicLight.h"
#include "GLSettings.h"

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
			GLFramebufferManager *fbManager;
			client::GameMap *map;
			GLSettings settings;

			bool inited;
			bool sceneUsedInThisFrame;

			client::SceneDefinition sceneDef;
			Plane3 frustrum[6];

			std::vector<DebugLine> debugLines;
			std::vector<GLDynamicLight> lights;

			GLProgramManager *programManager;
			GLImageManager *imageManager;
			GLModelManager *modelManager;

			IGLShadowMapRenderer *shadowMapRenderer;
			GLMapShadowRenderer *mapShadowRenderer;
			GLMapRenderer *mapRenderer;
			GLImageRenderer *imageRenderer;
			GLFlatMapRenderer *flatMapRenderer;
			GLModelRenderer *modelRenderer;
			IGLSpriteRenderer *spriteRenderer;
			GLLongSpriteRenderer *longSpriteRenderer;
			GLWaterRenderer *waterRenderer;
			GLAmbientShadowRenderer *ambientShadowRenderer;
			GLRadiosityRenderer *radiosityRenderer;

			GLCameraBlurFilter *cameraBlur;
			GLLensDustFilter *lensDustFilter;
			GLAutoExposureFilter *autoExposureFilter;

			// used when r_srgb = 1
			IGLDevice::UInteger lastColorBufferTexture;

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

		protected:
			virtual ~GLRenderer();

		public:
			GLRenderer(IGLDevice *glDevice);

			virtual void Init();
			virtual void Shutdown();

			virtual client::IImage *RegisterImage(const char *filename);
			virtual client::IModel *RegisterModel(const char *filename);

			virtual client::IImage *CreateImage(Bitmap *);
			virtual client::IModel *CreateModel(VoxelModel *);
			virtual client::IModel *CreateModelOptimized(VoxelModel *);

			GLProgram *RegisterProgram(const std::string &name);
			GLShader *RegisterShader(const std::string &name);

			virtual void SetGameMap(client::GameMap *);
			virtual void SetFogColor(Vector3 v);
			virtual void SetFogDistance(float f) { fogDistance = f; }

			Vector3 GetFogColor() { return fogColor; }
			float GetFogDistance() { return fogDistance; }
			Vector3 GetFogColorForSolidPass();

			virtual void StartScene(const client::SceneDefinition &);

			virtual void RenderModel(client::IModel *, const client::ModelRenderParam &);

			virtual void AddLight(const client::DynamicLightParam &light);

			virtual void AddDebugLine(Vector3 a, Vector3 b, Vector4 color);

			virtual void AddSprite(client::IImage *, Vector3 center, float radius, float rotation);
			virtual void AddLongSprite(client::IImage *, Vector3 p1, Vector3 p2, float radius);

			virtual void EndScene();

			virtual void MultiplyScreenColor(Vector3);

			virtual void SetColor(Vector4);
			virtual void SetColorAlphaPremultiplied(Vector4);

			virtual void DrawImage(client::IImage *, const Vector2 &outTopLeft);
			virtual void DrawImage(client::IImage *, const AABB2 &outRect);
			virtual void DrawImage(client::IImage *, const Vector2 &outTopLeft,
			                       const AABB2 &inRect);
			virtual void DrawImage(client::IImage *, const AABB2 &outRect, const AABB2 &inRect);
			virtual void DrawImage(client::IImage *, const Vector2 &outTopLeft,
			                       const Vector2 &outTopRight, const Vector2 &outBottomLeft,
			                       const AABB2 &inRect);

			virtual void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect);

			virtual void FrameDone();
			virtual void Flip();
			virtual Bitmap *ReadBitmap();

			virtual float ScreenWidth();
			virtual float ScreenHeight();

			GLSettings &GetSettings() { return settings; }
			IGLDevice *GetGLDevice() { return device; }
			GLFramebufferManager *GetFramebufferManager() { return fbManager; }
			IGLShadowMapRenderer *GetShadowMapRenderer() { return shadowMapRenderer; }
			GLAmbientShadowRenderer *GetAmbientShadowRenderer() { return ambientShadowRenderer; }
			GLMapShadowRenderer *GetMapShadowRenderer() { return mapShadowRenderer; }
			GLRadiosityRenderer *GetRadiosityRenderer() { return radiosityRenderer; }
			GLModelRenderer *GetModelRenderer() { return modelRenderer; }

			const Matrix4 &GetProjectionMatrix() const { return projectionMatrix; }
			const Matrix4 &GetProjectionViewMatrix() const { return projectionViewMatrix; }
			const Matrix4 &GetViewMatrix() const { return viewMatrix; }

			bool IsRenderingMirror() const { return renderingMirror; }

			virtual void GameMapChanged(int x, int y, int z, client::GameMap *);

			const client::SceneDefinition &GetSceneDef() const { return sceneDef; }

			bool BoxFrustrumCull(const AABB3 &);
			bool SphereFrustrumCull(const Vector3 &center, float radius);
		};
	}
}
