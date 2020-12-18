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

#include <array>

#include <Core/Math.h>
#include "IImage.h"
#include "IModel.h"
#include "SceneDefinition.h"
#include <Core/RefCountedObject.h>

namespace spades {
	class Bitmap;
	class VoxelModel;

	namespace client {

		class GameMap;

		struct ModelRenderParam {
			/** The transformatrix matrix applied on the model. */
			Matrix4 matrix = Matrix4::Identity();
			/** Voxels having a color value `(0, 0, 0)` are replaced with
			 *  this color. */
			Vector3 customColor = MakeVector3(0, 0, 0);
			/** Specifies to render the model in front of other non-depth-hack
			 *  models. Useful for first-person models. */
			bool depthHack = false;
			/** Specifies whether the model casts a shadow. */
			bool castShadow = true;
			/**
			 * Specifies that the model is not an actual object in the virtual world, thus does not
			 * affect the shading of other objects and does not appear in a mirror.
			 *
			 * This excludes the model from visual effects such as shadowing, global illumination
			 * (specifically, screen-space ambient occlusion, ATM), and dynamic lighting.
			 * In exchange, it allows the use of an opacity value other than `1`.
			 *
			 * `ghost` implies `!castShadow`.
			 */
			bool ghost = false;
			/** Specifies the opacity of the model. Ignored if `ghost` is `false`. */
			float opacity = 1.0;
			/** Specifies if the player is associated with the model, otherwise it is `-1`. */
			int playerID = -1;
			/** Specifies a player's team ID, ignored if `playerID` is set to `-1`. */
			unsigned char teamId : 1;
		};

		enum DynamicLightType { DynamicLightTypePoint, DynamicLightTypeSpotlight };

		struct DynamicLightParam {
			DynamicLightType type = DynamicLightTypePoint;
			/** The position of the light. */
			Vector3 origin;
			/** The effective radius of the light. Objects outside this radius
			 * is unaffected by the light. */
			float radius;
			Vector3 color;

			/** The basis vectors specifying the orientation of a spotlight.
			 *  See the existing code for usage. */
			std::array<Vector3, 3> spotAxis;
			/** The projected image for a spotlight. */
			IImage *image = nullptr;
			float spotAngle = 0.0f;

			/** When set to `true`, the lens flare post-effect is enabled for
			 * the light. */
			bool useLensFlare = false;
		};

		class IRenderer : public RefCountedObject {
		protected:
			virtual ~IRenderer() {}

		public:
			IRenderer() {}

			virtual void Init() = 0;
			virtual void Shutdown() = 0;

			virtual IImage *RegisterImage(const char *filename) = 0;
			virtual IModel *RegisterModel(const char *filename) = 0;

			/**
			 * Clear the cache of models and images loaded via `RegisterModel`
			 * and `RegisterImage`. This method is merely a hint - the
			 * implementation may partially or completely ignore the request.
			 */
			virtual void ClearCache() {}

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

			virtual void MultiplyScreenColor(Vector3) = 0;

			/** Sets color for image drawing. Deprecated because
			 * some methods treats this as an alpha premultiplied, while
			 * others treats this as an alpha non-premultiplied.
			 * @deprecated */
			virtual void SetColor(Vector4) = 0;

			/** Sets color for image drawing. Always alpha premultiplied. */
			virtual void SetColorAlphaPremultiplied(Vector4) = 0;

			virtual void DrawImage(IImage *, const Vector2 &outTopLeft) = 0;
			virtual void DrawImage(IImage *, const AABB2 &outRect) = 0;
			virtual void DrawImage(IImage *, const Vector2 &outTopLeft, const AABB2 &inRect) = 0;
			virtual void DrawImage(IImage *, const AABB2 &outRect, const AABB2 &inRect) = 0;
			virtual void DrawImage(IImage *, const Vector2 &outTopLeft, const Vector2 &outTopRight,
			                       const Vector2 &outBottomLeft, const AABB2 &inRect) = 0;

			virtual void DrawFlatGameMap(const AABB2 &outRect, const AABB2 &inRect) = 0;

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
