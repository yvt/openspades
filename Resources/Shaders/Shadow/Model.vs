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


uniform mat4 shadowMapMatrix1;
uniform mat4 shadowMapMatrix2;
uniform mat4 shadowMapMatrix3;
uniform mat4 shadowMapViewMatrix;

varying vec4 shadowMapCoord1;
varying vec4 shadowMapCoord2;
varying vec4 shadowMapCoord3;
// varying vec4 shadowMapViewPos;
#define shadowMapViewPosZ shadowMapCoord1.w

void TransformShadowMatrix(out vec4 shadowMapCoord,
						   in vec3 vertexCoord,
						   in mat4 matrix) {
	vec4 c;
	c = matrix * vec4(vertexCoord, 1.);
	c.xyz = (c.xyz * 0.5) + c.w * 0.5;
	// bias
	c.z -= c.w * 0.0003;
	shadowMapCoord = c;
}

void PrepareForShadow_Model(vec3 vertexCoord, vec3 normal) {
	vec4 shadowMapViewPos = shadowMapViewMatrix * vec4(vertexCoord, 1.);
	TransformShadowMatrix(shadowMapCoord1,
						  vertexCoord,
						  shadowMapMatrix1);
	TransformShadowMatrix(shadowMapCoord2,
						  vertexCoord,
						  shadowMapMatrix2);
	TransformShadowMatrix(shadowMapCoord3,
						  vertexCoord,
						  shadowMapMatrix3);
	shadowMapViewPosZ = shadowMapViewPos.z;
	
}