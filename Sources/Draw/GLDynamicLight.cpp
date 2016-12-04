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

#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		GLDynamicLight::GLDynamicLight(const client::DynamicLightParam &param) : param(param) {

			if (param.type == client::DynamicLightTypeSpotlight) {
				float t = tanf(param.spotAngle * .5f) * 2.f;
				Matrix4 mat;
				mat = Matrix4::FromAxis(param.spotAxis[0], param.spotAxis[1], param.spotAxis[2],
				                        param.origin);
				mat = mat * Matrix4::Scale(t, t, 1.f);

				projMatrix = mat.InversedFast();

				Matrix4 m = Matrix4::Identity();
				m.m[15] = 0.f;
				m.m[11] = 1.f;

				m.m[8] += .5f;
				m.m[9] += .5f;
				projMatrix = m * projMatrix;
			}
		}
	}
}
