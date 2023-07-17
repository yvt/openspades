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

#include <Client/IRenderer.h>
#include <Core/Math.h>
#include <array>

namespace spades {
	namespace draw {
		class GLDynamicLight {
			client::DynamicLightParam param;
			Matrix4 projMatrix;

			/** World-space clip planes (spotlight only) */
			std::array<Plane3, 4> clipPlanes;

			/** `(point2 - origin).GetPoweredLength()` (linear light only) */
			float poweredLength;

		public:
			GLDynamicLight(const client::DynamicLightParam &param);
			const client::DynamicLightParam &GetParam() const { return param; }

			const Matrix4 &GetProjectionMatrix() const { return projMatrix; }

			bool Cull(const AABB3 &) const;

			bool SphereCull(const Vector3 &center, float radius) const;
		};
	} // namespace draw
} // namespace spades
