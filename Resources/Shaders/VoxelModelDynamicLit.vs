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
uniform vec3 sunLightDirection;
uniform vec3 customColor;
uniform vec3 viewOriginVector;

// [x, y, z, AO ID]
attribute vec4 positionAttribute;

// [R, G, B, diffuse]
attribute vec4 colorAttribute;

// [x, y, z]
attribute vec3 normalAttribute;

varying vec4 color;
varying vec3 fogDensity;
//varying vec2 detailCoord;

void PrepareForDynamicLightNoBump(vec3 vertexCoord, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {

	vec4 vertexPos = vec4(positionAttribute.xyz, 1.);

	vertexPos.xyz += modelOrigin;

	gl_Position = projectionViewModelMatrix * vertexPos;

	color = colorAttribute;

	if(dot(color.xyz, vec3(1.)) < 0.0001){
		color.xyz = customColor;
	}

	// linearize
	color.xyz *= color.xyz;

	// material type (see `VoxelModel.h` for the definition)
	float materialType = colorAttribute.w * 255.0;

	if (materialType > 0.5) {
		// emissive material - unaffected by dynamic lights
		color.xyz = vec3(0.0);
	}

	// calculate normal
	vec3 normal = normalAttribute;
	normal = (modelNormalMatrix * vec4(normal, 1.)).xyz;
	normal = normalize(normal);

	vec2 horzRelativePos = (modelMatrix * vertexPos).xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

	PrepareForDynamicLightNoBump((modelMatrix * vertexPos).xyz, normal);
}

