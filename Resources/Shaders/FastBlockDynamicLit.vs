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
uniform vec3 viewPos;
uniform float pointSizeFactor;

// --- Vertex attribute ---
// [x, y, z, u]
attribute vec4 positionAttribute;

// [R, G, B, v]
attribute vec4 colorAttribute;

varying vec4 color;
varying vec3 fogDensity;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {
	
	vec4 vertexPos = vec4(chunkPosition, 1.);
	
	vertexPos.xyz += positionAttribute.xyz + 0.5;
	
	// calculate effective normal and tangents
	vec3 centerPos = vertexPos.xyz;
	vec3 viewRelPos = vertexPos.xyz - viewPos;
	vec3 normal = normalize(-viewRelPos);
	
	gl_Position = projectionViewMatrix * vertexPos;
	
	// color
	color = colorAttribute;
	color.xyz *= color.xyz; // linearize
	
	vec4 viewPos = viewMatrix * vertexPos;
	float distance = dot(viewPos.xyz, viewPos.xyz);
	fogDensity = FogDensity(distance).xyz;
	
	gl_PointSize = pointSizeFactor / viewPos.z;
	
	vec3 shadowVertexPos = vertexPos.xyz;
	PrepareForDynamicLightNoBump(shadowVertexPos, normal);
}

