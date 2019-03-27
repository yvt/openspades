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

#include "IImage.h"
#include "IModel.h"
#include "SceneDefinition.h"
#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	class Bitmap;
	class VoxelModel;

	namespace client {

		class GameMap;

		struct ModelRenderParam {
			Matrix4 matrix;
			Vector3 customColor;
			bool depthHack;
			
			int playerID = -1;
			bool teamId;
			
			ModelRenderParam() {
				matrix = Matrix4::Identity();
				customColor = MakeVector3(0, 0, 0);
				depthHack = false;
			}
		};

		enum DynamicLightType { DynamicLightTypePoint, DynamicLightTypeSpotlight };

		struct DynamicLightParam {
			DynamicLightType type;
			Vector3 origin;
			float radius;
			Vector3 color;

			Vector3 spotAxis[3];
			IImage *image;
			float spotAngle;

			bool useLensFlare;

			DynamicLightParam() {
				image = NULL;
				type = DynamicLightTypePoint;
				spotAngle = 0.f;
				useLensFlare = false;
			}
		};

		class IRenderer : public RefCountedObject {
		protected:
			virtual ~IRenderer() {}
			IRenderer();

			virtual void SetScissorLowLevel(const AABB2 &) = 0;

			virtual void DrawImageLowLevel(IImage *, const AABB2 &outRect, const AABB2 &inRect);
			virtual void DrawImageLowLevel(IImage *, const Vector2 &outTopLeft,
			                               const Vector2 &outTopRight, const Vector2 &outBottomLeft,
			                               const AABB2 &inRect) = 0;

			virtual void InitLowLevel() = 0;
			virtual void ShutdownLowLevel() = 0;

		public:
			void Init();
			void Shutdown();

			virtual IImage *RegisterImage(const char *filename) = 0;
			virtual IModel *RegisterModel(const char *filename) = 0;

			virtual IImage *CreateImage(Bitmap *) = 0;
			virtual IModel *CreateModel(VoxelModel *) = 0;

			virtual void SetGameMap(GameMap *) = 0;

			virtual void SetFogDistance(float) = 0;
			virtual void SetFogColor(Vector3) = 0;

			/** Starts rendering a scene and waits for additional objects. */
			virtual void StartScene(const SceneDefinition &) = 0;

			virtual void AddLight(const client::DynamicLightParam &light) = 0;

			virtual void RenderModel(IModel *, const ModelRenderParam &) = 0;
			virtual void AddDebugLine(Vector3 a, Vector3 b, Vector4 color) = 0;

			virtual void AddSprite(IImage *, Vector3 center, float radius, float rotation) = 0;
			virtual void AddLongSprite(IImage *, Vector3 p1, Vector3 p2, float radius) = 0;

			/** Finalizes a scene. 2D drawing follows. */
			virtual void EndScene() = 0;

			/** Saves the current 2D drawing state. */
			void Save();

			/** Restores the previously saved 2D drawing state. */
			void Restore();

			/** Modifies the current transform matrix by multiplying a scaling matrix. */
			void Scale(const Vector2 &);

			void Scale(float factor) { Scale(Vector2{factor, factor}); }

			/** Modifies the current transform matrix by multiplying a translation matrix. */
			void Translate(const Vector2 &);

			virtual void MultiplyScreenColor(Vector3) = 0;

			/** Sets color for image drawing. Deprecated because
			 * some drawing functions treat this alpha premultiplied, while
			 * others treat this alpha non-premultiplied.
			 * @deprecated */
			virtual void SetColor(Vector4) = 0;

			/** Sets color for image drawing. Always alpha premultiplied.
			 * The color is specified in the sRGB color space (not linear color space). */
			virtual void SetColorAlphaPremultiplied(Vector4) = 0;

			void SetScissor(const AABB2 &);

			void DrawImage(IImage *, const Vector2 &outTopLeft);
			void DrawImage(IImage *, const AABB2 &outRect);
			void DrawImage(IImage *, const Vector2 &outTopLeft, const AABB2 &inRect);
			void DrawImage(IImage *, const AABB2 &outRect, const AABB2 &inRect);
			void DrawImage(IImage *, const Vector2 &outTopLeft, const Vector2 &outTopRight,
			               const Vector2 &outBottomLeft, const AABB2 &inRect);

			/* Applies a frosting glass effect (if possible). */
			virtual void Blur() {}

			virtual void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect) = 0;

			/** Finalizes a frame. */
			virtual void FrameDone() = 0;

			/** displays a rendered image to the screen. */
			virtual void Flip() = 0;

			/** get a rendered image. */
			virtual Bitmap *ReadBitmap() = 0;

			virtual float ScreenWidth() = 0;
			virtual float ScreenHeight() = 0;

		private:
			struct State2D {
				AABB2 scissorRect;
				Vector2 transformScaling;
				Vector2 transformTranslation;
			};
			std::vector<State2D> m_stateStack;

			void ResetStack();
			State2D &GetCurrentState();

			Vector2 TransformPoint(const Vector2 &);
			Vector2 TransformVector(const Vector2 &);
			AABB2 Transform(const AABB2 &);
		};
	}
}
