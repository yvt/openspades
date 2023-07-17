/*
 Copyright (c) 2021 yvt

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
#include <cmath>

#include "SceneDefinition.h"

namespace spades {
	namespace client {
		Matrix4 SceneDefinition::ToOpenGLProjectionMatrix() const {
			float near = this->zNear;
			float far = this->zFar;
			float t = near * std::tan(this->fovY * .5f);
			float r = near * std::tan(this->fovX * .5f);
			float a = r * 2.f, b = t * 2.f, c = far - near;

			Matrix4 mat;
			mat.m[0] = near * 2.f / a;
			mat.m[1] = 0.f;
			mat.m[2] = 0.f;
			mat.m[3] = 0.f;
			mat.m[4] = 0.f;
			mat.m[5] = near * 2.f / b;
			mat.m[6] = 0.f;
			mat.m[7] = 0.f;
			mat.m[8] = 0.f;
			mat.m[9] = 0.f;
			mat.m[10] = -(far + near) / c;
			mat.m[11] = -1.f;
			mat.m[12] = 0.f;
			mat.m[13] = 0.f;
			mat.m[14] = -(far * near * 2.f) / c;
			mat.m[15] = 0.f;

			return mat;
		}

		Matrix4 SceneDefinition::ToViewMatrix() const {
			Matrix4 mat = Matrix4::Identity();
			mat.m[0] = this->viewAxis[0].x;
			mat.m[4] = this->viewAxis[0].y;
			mat.m[8] = this->viewAxis[0].z;
			mat.m[1] = this->viewAxis[1].x;
			mat.m[5] = this->viewAxis[1].y;
			mat.m[9] = this->viewAxis[1].z;
			mat.m[2] = -this->viewAxis[2].x;
			mat.m[6] = -this->viewAxis[2].y;
			mat.m[10] = -this->viewAxis[2].z;

			Vector4 v = mat * this->viewOrigin;
			mat.m[12] = -v.x;
			mat.m[13] = -v.y;
			mat.m[14] = -v.z;

			return mat;
		}
	} // namespace client
} // namespace spades
