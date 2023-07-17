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

/**** CPU RADIOSITY (FASTER?) *****/

uniform sampler3D ambientShadowTexture;

varying vec3 radiosityTextureCoord;
varying vec3 ambientShadowTextureCoord;
varying vec3 normalVarying;

void PrepareForRadiosity_Map(vec3 vertexCoord, vec3 normal) {
	radiosityTextureCoord = (vertexCoord + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
	ambientShadowTextureCoord = (vertexCoord + vec3(0., 0., 1.)) / vec3(512., 512., 65.);

	normalVarying = normal;
}

void PrepareForRadiosityForMap_Map(vec3 vertexCoord, vec3 centerCoord, vec3 normal) {
	radiosityTextureCoord = (vertexCoord + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
	ambientShadowTextureCoord = (vertexCoord + vec3(0., 0., 1.) + normal * 0.5) / vec3(512., 512., 65.);

	vec3 centerAST = (centerCoord + vec3(0., 0., 1.) + normal * 0.5) / vec3(512., 512., 65.);
	vec3 rel = vertexCoord - centerCoord;
	vec3 relAST = rel * 2.0 / vec3(512., 512., 65.);

	// Detect the following pattern:
	//
	//      +-----+-----+
	//      |#####|     |
	//      |#####|     |
	//      |#####|     |
	//      +-----+-----+
	//      |    V|#####|
	//      |  C  |#####|
	//      |     |#####|
	//      +-----+-----+
	//
	// C = centerCoord, V = vertexCoord, # = covered by a solid voxel
	//
	float weightSum;
	if (normal.x != 0.0) {
		weightSum = texture3D(ambientShadowTexture, centerAST + vec3(0.0, relAST.y, 0.0)).y +
		          texture3D(ambientShadowTexture, centerAST + vec3(0.0, 0.0, relAST.z)).y -
		          texture3D(ambientShadowTexture, centerAST + vec3(0.0, relAST.y, relAST.z)).y;
	} else if (normal.y != 0.0) {
		weightSum = texture3D(ambientShadowTexture, centerAST + vec3(relAST.x, 0.0, 0.0)).y +
		          texture3D(ambientShadowTexture, centerAST + vec3(0.0, 0.0, relAST.z)).y -
		          texture3D(ambientShadowTexture, centerAST + vec3(relAST.x, 0.0, relAST.z)).y;
	} else {
		weightSum = texture3D(ambientShadowTexture, centerAST + vec3(relAST.x, 0.0, 0.0)).y +
		          texture3D(ambientShadowTexture, centerAST + vec3(0.0, relAST.y, 0.0)).y -
		          texture3D(ambientShadowTexture, centerAST + vec3(relAST.x, relAST.y, 0.0)).y;
	}

	// Hide the light leaks by corners by modifying the AO texture coordinates
	if (weightSum < -0.5) {
		radiosityTextureCoord -= rel / vec3(512., 512., 64.);
		ambientShadowTextureCoord = centerAST;
	}

	normalVarying = normal;
}