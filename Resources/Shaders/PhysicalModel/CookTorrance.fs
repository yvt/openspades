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

float BeckmannDistribution(float m, float dotHalf) {
	float m2 = m * m;
	float dot2 = dotHalf * dotHalf;
	float ep = (dot2 - 1.) / (dot2 * m2);
	ep = exp(ep);

	return ep / (3.141592654 * m2 * dot2 * dot2);
}

float GGXDistribution(float m, float dotHalf) {
	float m2 = m * m;
	float dot2 = dotHalf * dotHalf;
	float t = dot2 * (m2 - 1.) + 1.;
	return m2 / (3.141592653 * t * t);
}

// http://en.wikipedia.org/wiki/Specular_highlight#Cook.E2.80.93Torrance_model
float CockTorrance(vec3 eyeVec, vec3 lightVec, vec3 normal) {
	float LN = dot(lightVec, normal);
	if(LN <= 0.) return 0.;

	vec3 halfVec = lightVec + eyeVec;
	halfVec = dot(halfVec, halfVec) < .00000000001 ? vec3(1., 0., 0.) : normalize(halfVec);

	// distribution term
	float distribution = dot(halfVec, normal);
	float m = .3; // roughness
	distribution = GGXDistribution(m, distribution);

	// fresnel term
	// FIXME: use split-sum approximation from UE4
	float fresnel2 = 1. - dot(halfVec, eyeVec);
	float fresnel = fresnel2 * fresnel2;

	fresnel = .03 + fresnel * 0.1;

	// visibility term
	float a = m * 0.7978, ia = 1. - a;
	float dot1 = dot(lightVec, normal);
	float dot2 = dot(eyeVec, normal);
	float visibility = (dot1 * ia + a) * (dot2 * ia + a);
	visibility = .25 / visibility;

	float specular = distribution * fresnel * visibility;
	return specular;
}


