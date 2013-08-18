//
//  IRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IImage.h"
#include "../Core/Math.h"
#include "SceneDefinition.h"
#include "IModel.h"

namespace spades {
	class Bitmap;
	class VoxelModel;
	
	namespace client {
		
		class GameMap;
		
		struct ModelRenderParam {
			Matrix4 matrix;
			Vector3 customColor;
			bool depthHack;
			
			ModelRenderParam() {
				matrix = Matrix4::Identity();
				customColor = MakeVector3(0, 0, 0);
				depthHack = false;
			}
		};
		
		enum DynamicLightType {
			DynamicLightTypePoint,
			DynamicLightTypeSpotlight
		};
		
		struct DynamicLightParam {
			DynamicLightType type;
			Vector3 origin;
			float radius;
			Vector3 color;
			
			Vector3 spotAxis[3];
			IImage *image;
			float spotAngle;
			
			DynamicLightParam() {
				image = NULL;
				type = DynamicLightTypePoint;
				spotAngle = 0.f;
			}
		};
		
		class IRenderer {
		public:
			
			virtual ~IRenderer() {}
			
			virtual IImage *RegisterImage(const char *filename) = 0;
			virtual IModel *RegisterModel(const char *filename) = 0;
			
			virtual IImage *CreateImage(Bitmap *) = 0;
			virtual IModel *CreateModel(VoxelModel *) = 0;
			
			virtual void SetGameMap(GameMap *) = 0;
			
			virtual void SetFogDistance(float) = 0;
			virtual void SetFogColor(Vector3) = 0;
			
			/** Starts rendering a scene and waits for additional objects. */
			virtual void StartScene(const SceneDefinition&) = 0;
			
			virtual void AddLight(const client::DynamicLightParam& light) = 0;
			
			virtual void RenderModel(IModel *, const ModelRenderParam&) = 0;
			virtual void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) = 0;
			
			virtual void AddSprite(IImage *, Vector3 center, float radius, float rotation) = 0;
			
			/** Finalizes a scene. 2D drawing follows. */
			virtual void EndScene() = 0;
			
			virtual void MultiplyScreenColor(Vector3) = 0;
			
			/** Sets color for image drawing. */
			virtual void SetColor(Vector4) = 0;
			
			virtual void DrawImage(IImage *, const Vector2& outTopLeft) = 0;
			virtual void DrawImage(IImage *, const AABB2& outRect) = 0;
			virtual void DrawImage(IImage *, const Vector2& outTopLeft, const AABB2& inRect) = 0;
			virtual void DrawImage(IImage *, const AABB2& outRect, const AABB2& inRect) = 0;
			virtual void DrawImage(IImage *, const Vector2& outTopLeft, const Vector2& outTopRight, const Vector2& outBottomLeft, const AABB2& inRect) = 0;
			
			virtual void DrawFlatGameMap(const AABB2& outRect, const AABB2& inRect) = 0;
			
			/** Finalizes a frame. */
			virtual void FrameDone() = 0;
			
			/** displays a rendered image to the screen. */
			virtual void Flip() = 0;
			
			/** get a rendered image. */
			virtual Bitmap *ReadBitmap() = 0;
			
			virtual float ScreenWidth() = 0;
			virtual float ScreenHeight() = 0;
		};

	}
}

