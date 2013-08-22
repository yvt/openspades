//
//  GLRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IRenderer.h"
#include "../Core/Math.h"
#include "../Client/SceneDefinition.h"
#include <map>
#include "../Client/IGameMapListener.h"
#include "GLCameraBlurFilter.h"
#include "GLDynamicLight.h"

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
		class GLFramebufferManager;
		class GLMapShadowRenderer;
		class GLModelRenderer;
		class IGLShadowMapRenderer;
		class GLWaterRenderer;
		class GLAmbientShadowRenderer;
		class GLRadiosityRenderer;
		
		class GLRenderer: public client::IRenderer, public client::IGameMapListener  {
			friend class GLShadowShader;
			friend class IGLShadowMapRenderer;
			friend class GLRadiosityRenderer;
			
			struct DebugLine{
				Vector3 v1, v2;
				Vector4 color;
			};
			
			IGLDevice *device;
			GLFramebufferManager *fbManager;
			
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
			GLWaterRenderer *waterRenderer;
			GLAmbientShadowRenderer *ambientShadowRenderer;
			GLRadiosityRenderer *radiosityRenderer;
			
			GLCameraBlurFilter cameraBlur;
			
			float fogDistance;
			Vector3 fogColor;
			
			Matrix4 projectionMatrix;
			Matrix4 viewMatrix;
			Matrix4 projectionViewMatrix;
			
			Vector4 drawColor;
			
			unsigned int lastTime;
			
			void BuildProjectionMatrix();
			void BuildView();
			void BuildFrustrum();
			
			void RenderDebugLines();
			
		public:
			GLRenderer(IGLDevice *glDevice);
			virtual ~GLRenderer();
			
			virtual client::IImage *RegisterImage(const char *filename);
			virtual client::IModel *RegisterModel(const char *filename);
			
			virtual client::IImage *CreateImage(Bitmap *);
			virtual client::IModel *CreateModel(VoxelModel *);
			virtual client::IModel *CreateModelOptimized(VoxelModel *);
			
			GLProgram *RegisterProgram(const std::string& name);
			GLShader *RegisterShader(const std::string& name);
			
			virtual void SetGameMap(client::GameMap *);
			virtual void SetFogColor(Vector3 v){fogColor = v;}
			virtual void SetFogDistance(float f){fogDistance = f;}
			
			Vector3 GetFogColor() { return fogColor; }
			float GetFogDistance() { return fogDistance; }
			Vector3 GetFogColorForSolidPass();
			
			virtual void StartScene(const client::SceneDefinition&);
			
			virtual void RenderModel(client::IModel *, const client::ModelRenderParam&);
			
			virtual void AddLight(const client::DynamicLightParam& light);
			
			virtual void AddDebugLine(Vector3 a, Vector3 b, Vector4 color);
			
			virtual void AddSprite(client::IImage *, Vector3 center, float radius, float rotation);
			
			virtual void EndScene();
			
			
			virtual void MultiplyScreenColor(Vector3);
			
			virtual void SetColor(Vector4);
			
			virtual void DrawImage(client::IImage *, const Vector2& outTopLeft);
			virtual void DrawImage(client::IImage *, const AABB2& outRect);
			virtual void DrawImage(client::IImage *, const Vector2& outTopLeft, const AABB2& inRect);
			virtual void DrawImage(client::IImage *, const AABB2& outRect, const AABB2& inRect);
			virtual void DrawImage(client::IImage *, const Vector2& outTopLeft, const Vector2& outTopRight, const Vector2& outBottomLeft, const AABB2& inRect);
			
			virtual void DrawFlatGameMap(const AABB2& outRect, const AABB2& inRect);
			
			virtual void FrameDone();
			virtual void Flip();
			virtual Bitmap *ReadBitmap();
			
			virtual float ScreenWidth();
			virtual float ScreenHeight();
			
			IGLDevice *GetGLDevice() {return device; }
			GLFramebufferManager *GetFramebufferManager() { return fbManager; }
			IGLShadowMapRenderer *GetShadowMapRenderer() { return shadowMapRenderer; }
			GLAmbientShadowRenderer *GetAmbientShadowRenderer() { return ambientShadowRenderer; }
			GLMapShadowRenderer *GetMapShadowRenderer() { return mapShadowRenderer; }
			GLRadiosityRenderer *GetRadiosityRenderer() { return radiosityRenderer; }
			
			const Matrix4& GetProjectionMatrix() const { return projectionMatrix; }
			const Matrix4& GetProjectionViewMatrix() const { return projectionViewMatrix; }
			const Matrix4& GetViewMatrix() const { return viewMatrix; }
			
			virtual void GameMapChanged(int x, int y, int z, client::GameMap *);
					
			const client::SceneDefinition& GetSceneDef() const {
				return sceneDef;
			}
			
			bool BoxFrustrumCull(const AABB3&);
			bool SphereFrustrumCull(const Vector3& center,
										float radius);
			
		};

	}
}

