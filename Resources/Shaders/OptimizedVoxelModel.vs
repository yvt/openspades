/*
 Copyright (c) 2013 OpenSpades Developers
 
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
uniform vec3 sunLightDirection;
uniform vec2 texScale;

// [x, y, z]
attribute vec3 positionAttribute;

// [u, v]
attribute vec2 textureCoordAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec4 textureCoord;
varying vec4 color;
varying vec3 fogDensity;
varying float flatShading;
//varying vec2 detailCoord;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);
	
	vertexPos.xyz += modelOrigin;
	
	gl_Position = projectionViewModelMatrix * vertexPos;
	
	textureCoord = textureCoordAttribute.xyxy * vec4(texScale.xy, vec2(1.));
	
	// direct sunlight
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);
	float sunlight = dot(normal, sunLightDirection);
	sunlight = max(sunlight, 0.);
	flatShading = sunlight;
	
	vec4 viewPos = viewModelMatrix * vertexPos;
	float distance = dot(viewPos.xyz, viewPos.xyz);
	fogDensity = FogDensity(distance).xyz;
	
	PrepareForShadow((modelMatrix * vertexPos).xyz, normal);
}

