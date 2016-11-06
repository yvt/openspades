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
uniform mat4 modelNormalMatrix;
uniform mat4 viewModelMatrix;
uniform vec3 modelOrigin;
uniform float fogDistance;
uniform vec2 texScale;
uniform vec3 viewOriginVector;

// [x, y, z]
attribute vec3 positionAttribute;

// [u, v]
attribute vec2 textureCoordAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec2 textureCoord;
varying vec3 fogDensity;
//varying vec2 detailCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {

	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);

	vertexPos.xyz += modelOrigin;

	gl_Position = projectionViewModelMatrix * vertexPos;

	textureCoord = textureCoordAttribute.xy * texScale.xy;

	vec4 viewPos = viewModelMatrix * vertexPos;
	vec2 horzRelativePos = (modelMatrix * vertexPos).xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

	// compute normal
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);

	PrepareForDynamicLightNoBump((modelMatrix * vertexPos).xyz, normal);
}

