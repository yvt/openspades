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
				float t = tanf(param.spotAngle * .5f);
				Matrix4 mat;
				mat = Matrix4::FromAxis(param.spotAxis[0], param.spotAxis[1], param.spotAxis[2],
				                        param.origin);
				mat = mat * Matrix4::Scale(t * 2.0f, t * 2.0f, 1.f);

				projMatrix = mat.InversedFast();

				Matrix4 m = Matrix4::Identity();
				m.m[15] = 0.f;
				m.m[11] = 1.f;

				m.m[8] += .5f;
				m.m[9] += .5f;
				projMatrix = m * projMatrix;

				// Construct clipping planes which are oriented inside.
				// To do that, first we calculate tangent vectors:
				Vector3 planeTan[] = {
				  param.spotAxis[2] + param.spotAxis[0] * t,
				  param.spotAxis[2] + param.spotAxis[1] * t,
				  param.spotAxis[2] - param.spotAxis[0] * t,
				  param.spotAxis[2] - param.spotAxis[1] * t,
				};
				// Then, use them to derive normal vectors:
				Vector3 planeN[] = {
				  Vector3::Cross(param.spotAxis[1], planeTan[0]),
				  Vector3::Cross(planeTan[1], param.spotAxis[0]),
				  Vector3::Cross(planeTan[2], param.spotAxis[1]),
				  Vector3::Cross(param.spotAxis[0], planeTan[3]),
				};
				// Finally, find planes with these normal vectors:
				for (std::size_t i = 0; i < 4; ++i) {
					clipPlanes[i] = Plane3::PlaneWithPointOnPlane(param.origin, planeN[i]);
				}
			}

			if (param.type == client::DynamicLightTypeLinear) {
				poweredLength = (param.point2 - param.origin).GetPoweredLength();
			}
		}

		bool GLDynamicLight::Cull(const spades::AABB3 &box) const {
			const client::DynamicLightParam &param = GetParam();

			if (param.type == client::DynamicLightTypeSpotlight) {
				for (const Plane3 &plane : clipPlanes) {
					if (!PlaneCullTest(plane, box)) {
						return false;
					}
				}
			}

			AABB3 inflatedBox = box.Inflate(param.radius);

			if (param.type == client::DynamicLightTypeLinear) {
				Vector3 intersection;
				// TODO: using `OBB3` here is overkill, but `AABB3` doesn't have `RayCast`
				if (!OBB3(inflatedBox)
				       .RayCast(param.origin, param.point2 - param.origin, &intersection)) {
					return false;
				}
				return (intersection - param.origin).GetPoweredLength() <= poweredLength;
			}

			return inflatedBox && param.origin;
		}

		bool GLDynamicLight::SphereCull(const spades::Vector3 &center, float radius) const {
			const client::DynamicLightParam &param = GetParam();

			if (param.type == client::DynamicLightTypeSpotlight) {
				for (const Plane3 &plane : clipPlanes) {
					if (plane.GetDistanceTo(center) < -radius) {
						return false;
					}
				}
			} else if (param.type == client::DynamicLightTypeLinear) {
				return Line3::MakeLineSegment(param.origin, param.point2).GetDistanceTo(center) <
				       radius + param.radius;
			}

			float maxDistance = radius + param.radius;
			return (center - param.origin).GetPoweredLength() < maxDistance * maxDistance;
		}
	} // namespace draw
} // namespace spades
