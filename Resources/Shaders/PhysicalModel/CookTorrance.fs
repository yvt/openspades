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

// http://en.wikipedia.org/wiki/Specular_highlight#Cook.E2.80.93Torrance_model
float CockTorrance(vec3 eyeVec, vec3 lightVec, vec3 normal) {
	float LN = dot(lightVec, normal);
	if(LN <= 0.) return 0.;
	
	vec3 halfVec = normalize(lightVec + eyeVec);
	
	// distribution term
	float distribution = dot(halfVec, normal);
	const float power = 8.;
	distribution *= distribution;
	distribution *= distribution;
	distribution *= distribution;
	distribution *= (power + 2.) / 3.141592654;
	
	// fresnel term
	float fresnel2 = 1. - dot(halfVec, eyeVec);
	float fresnel = fresnel2 * fresnel2;
	fresnel *= fresnel * fresnel2;
	
	fresnel = .03 + fresnel * 0.5;
	
	// visibility term
	vec3 v = lightVec + eyeVec;
	float visibility = 1. / dot(v, v);
	
	float specular = distribution * fresnel * visibility;
	return specular;
}


