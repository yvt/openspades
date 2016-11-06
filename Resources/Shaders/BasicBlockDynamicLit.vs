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



uniform mat4 projectionViewMatrix;
uniform mat4 viewMatrix;
uniform vec3 chunkPosition;
uniform float fogDistance;
uniform vec3 viewOriginVector;

// --- Vertex attribute ---
// [x, y, z]
attribute vec3 positionAttribute;

// [R, G, B, diffuse]
attribute vec4 colorAttribute;

// [nx, ny, nz]
attribute vec3 normalAttribute;

varying vec4 color;
varying vec3 fogDensity;
varying vec2 detailCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {

	vec4 vertexPos = vec4(chunkPosition, 1.);

	vertexPos.xyz += positionAttribute.xyz;

	gl_Position = projectionViewMatrix * vertexPos;

	color = colorAttribute;
	color.xyz *= color.xyz; // linearize

	vec4 viewPos = viewMatrix * vertexPos;
	vec2 horzRelativePos = vertexPos.xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

	vec3 normal = normalAttribute;
	vec3 shadowVertexPos = vertexPos.xyz;

	PrepareForDynamicLightNoBump(shadowVertexPos, normal);
}

