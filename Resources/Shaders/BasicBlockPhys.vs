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

// --- Vertex attribute ---
// [x, y, z]
attribute vec3 positionAttribute;

// [ax, ay]
attribute vec2 ambientOcclusionCoordAttribute;

// [R, G, B, diffuse]
attribute vec4 colorAttribute;

// [nx, ny, nz]
attribute vec3 normalAttribute;

varying vec2 ambientOcclusionCoord;
varying vec4 color;
varying vec3 fogDensity;

varying vec3 viewSpaceCoord;
varying vec3 viewSpaceNormal;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {
	
	vec4 vertexPos = vec4(chunkPosition, 1.);
	
	vertexPos.xyz += positionAttribute.xyz;
	
	gl_Position = projectionViewMatrix * vertexPos;
	
	color = colorAttribute;
	color.xyz *= color.xyz; // linearize
	
	// lambert reflection
	vec3 sunDir = normalize(vec3(0, -1., -1.));
	color.w = dot(sunDir, normalAttribute);
	
	
	// ambient occlusion
	ambientOcclusionCoord = (ambientOcclusionCoordAttribute + .5) * (1. / 256.);

	vec4 viewPos = viewMatrix * vertexPos;
	float distance = dot(viewPos.xyz, viewPos.xyz);
	fogDensity = FogDensity(distance).xyz;

	vec3 normal = normalAttribute;
	vec3 shadowVertexPos = vertexPos.xyz;
	if(abs(normal.x) > .1) // avoid self shadowing
		shadowVertexPos += normal * 0.01;
	PrepareForShadow(shadowVertexPos, normal);
	
	
	// used for diffuse lighting
	viewSpaceCoord = viewPos.xyz;
	viewSpaceNormal = (viewMatrix * vec4(normal, 0.)).xyz;
}

