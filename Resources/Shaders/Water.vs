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



uniform mat4 projectionViewModelMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
uniform vec3 viewOrigin;
uniform float fogDistance;

// [x, y]
attribute vec2 positionAttribute;

varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec3 worldPosition;

uniform sampler2D waveTexture;
//varying vec2 detailCoord;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {

	vec4 vertexPos = vec4(positionAttribute.xy, 0., 1.);

	worldPosition = (modelMatrix * vertexPos).xyz;

	gl_Position = projectionViewModelMatrix * vertexPos;
	screenPosition = gl_Position.xyw;
	screenPosition.xy = (screenPosition.xy + screenPosition.z) * .5;

	vec4 viewPos = viewModelMatrix * vertexPos;
	vec2 horzRelativePos = (modelMatrix * vertexPos).xy - viewOrigin.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

	viewPosition = viewPos.xyz;


	PrepareForShadow((modelMatrix * vertexPos).xyz, vec3(0., 0., -1.));
}

