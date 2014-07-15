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

#include "../Core/ConcurrentDispatch.h"
#include "IRenderer.h"
#include <map>

namespace spades {
	namespace client {
		class TemporaryAsyncImage;
		class TemporaryAsyncModel;
		class AsyncRenderer: public IRenderer {
			class RenderDispatch;
			class CmdBufferGenerator;
			class CmdBufferReader;
			friend class TemporaryAsyncImage;
			friend class TemporaryAsyncModel;
			
			IRenderer *base;
			DispatchQueue *queue;
			
			RenderDispatch *dispatch;
			CmdBufferGenerator *generator;
			
			std::map<std::string, IImage *> images;
			std::map<std::string, IModel *> models;
			
			void FlushCommands();
			void Sync();
		public:
			AsyncRenderer(IRenderer *base,
						  DispatchQueue *renderQueue);
			virtual ~AsyncRenderer();
			
			virtual void Init();
			virtual void Shutdown();
			
			virtual IImage *RegisterImage(const char *filename);
			virtual IModel *RegisterModel(const char *filename);
			
			virtual IImage *CreateImage(Bitmap *);
			virtual IModel *CreateModel(VoxelModel *);
			
			virtual void SetGameMap(GameMap *);
			
			virtual void SetFogDistance(float);
			virtual void SetFogColor(Vector3);
			virtual void SetFogType(client::FogType);
			
			/** Starts rendering a scene and waits for additional objects. */
			virtual void StartScene(const SceneDefinition&);
			
			virtual void AddLight(const client::DynamicLightParam& light);
			
			virtual void RenderModel(IModel *, const ModelRenderParam&);
			virtual void AddDebugLine(Vector3 a, Vector3 b, Vector4 color);
			
			virtual void AddSprite(IImage *, Vector3 center, float radius, float rotation);
			virtual void AddLongSprite(IImage *, Vector3 p1, Vector3 p2, float radius);
			
			/** Finalizes a scene. 2D drawing follows. */
			virtual void EndScene();
			
			virtual void MultiplyScreenColor(Vector3);
			
			/** Sets color for image drawing. */
			virtual void SetColor(Vector4);
			virtual void SetColorAlphaPremultiplied(Vector4);
			
			virtual void DrawImage(IImage *, const Vector2& outTopLeft);
			virtual void DrawImage(IImage *, const AABB2& outRect);
			virtual void DrawImage(IImage *, const Vector2& outTopLeft, const AABB2& inRect);
			virtual void DrawImage(IImage *, const AABB2& outRect, const AABB2& inRect);
			virtual void DrawImage(IImage *,
								   const Vector2& outTopLeft, const Vector2& outTopRight, const Vector2& outBottomLeft,
								   const AABB2& inRect);
			virtual void DrawImage(IImage *,
								   const Vector2& outTopLeft, const Vector2& outTopRight, const Vector2& outBottomLeft,
								   const Vector2& inTopLeft, const Vector2& inTopRight, const Vector2& inBottomLeft);
			
			virtual void DrawFlatGameMap(const AABB2& outRect, const AABB2& inRect);
			
			/** Finalizes a frame. */
			virtual void FrameDone() ;
			
			/** displays a rendered image to the screen. */
			virtual void Flip();
			
			/** get a rendered image. */
			virtual Bitmap *ReadBitmap();
			
			virtual float ScreenWidth();
			virtual float ScreenHeight();
			
		};
	}
}
