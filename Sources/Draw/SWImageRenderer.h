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

#include "SWFeatureLevel.h"
#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	class Bitmap;

	namespace draw {
		class SWImage;
		class SWImageRenderer {
		public:
			struct Vertex {
				Vector4 position;
				Vector4 color;
				Vector2 uv;
			};
			enum class ShaderType { Image, Sprite };

		private:
			Handle<Bitmap> frame;
			float *depthBuffer;
			ShaderType shader;
			Vector4 fbSize4;
			Vector4 fbCenter4;
			float zNear;
			Matrix4 matrix;
			SWFeatureLevel featureLevel;
			unsigned long long pixelsDrawn;

			template <SWFeatureLevel, bool, bool, bool, bool, bool> struct PolygonRenderer;

			template <SWFeatureLevel, bool, bool, bool, bool> struct PolygonRenderer3;

			template <bool, bool, bool, bool> struct PolygonRenderer2;

		public:
			SWImageRenderer(SWFeatureLevel);
			~SWImageRenderer();
			void SetFramebuffer(Bitmap *);
			void SetDepthBuffer(float *);

			void SetMatrix(const Matrix4 &m) { matrix = m; }
			void SetZRange(float zNear, float zFar);

			void SetShaderType(ShaderType);

			void DrawPolygon(SWImage *img, const Vertex &v1, const Vertex &v2, const Vertex &v3);

			unsigned long long GetPixelsDrawn() { return pixelsDrawn; }
			void ResetPixelStatistics() { pixelsDrawn = 0; }
		};
	} // namespace draw
} // namespace spades
