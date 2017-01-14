#extension GL_EXT_texture_array : require
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
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform vec3 viewOrigin;
uniform float fogDistance;

// [x, y]
attribute vec2 positionAttribute;

varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec3 worldPosition;
varying vec2 worldPositionOriginal;

uniform sampler2DArray waveTextureArray;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);
vec4 FogDensity(float poweredLength);

vec3 DisplaceWater(vec2 worldPos){

	vec4 waveCoord = worldPos.xyxy * vec4(vec2(0.08), vec2(0.15704))
	+ vec4(0., 0., 0.754, 0.1315);

	vec2 waveCoord2 = worldPos.xy * 0.02344 + vec2(.154, .7315);

	float wave = texture2DArrayLod(waveTextureArray, vec3(waveCoord.xy, 0.0), 0.).w;
	float disp = mix(-0.1, 0.1, wave) * 0.4;

	float wave2 = texture2DArrayLod(waveTextureArray, vec3(waveCoord.zw, 1.0), 0.).w;
	disp += mix(-0.1, 0.1, wave2) * 0.2;

	float wave3 = texture2DArrayLod(waveTextureArray, vec3(waveCoord2.xy, 2.0), 0.).w;
	disp += mix(-0.1, 0.1, wave3) * 2.5;

	float waveSmoothed1 = texture2DArrayLod(waveTextureArray, vec3(waveCoord2.xy, 2.0), 4.).w;
	float waveSmoothed2 = texture2DArrayLod(waveTextureArray, vec3(waveCoord2.xy + vec2(1.0 / 16.0, 0.0), 2.0), 3.).w;
	float waveSmoothed3 = texture2DArrayLod(waveTextureArray, vec3(waveCoord2.xy + vec2(0.0, 1.0 / 16.0), 2.0), 3.).w;
	vec2 dispHorz = vec2(waveSmoothed2 - waveSmoothed1, waveSmoothed3 - waveSmoothed1) * -16.;

	return vec3(dispHorz, disp * 4.);
}

void main() {

	vec4 vertexPos = vec4(positionAttribute.xy, 0., 1.);

	worldPosition = (modelMatrix * vertexPos).xyz;

	worldPositionOriginal = worldPosition.xy;
	worldPosition += DisplaceWater(worldPosition.xy);

	gl_Position = projectionViewMatrix * vec4(worldPosition, 1.);
	screenPosition = gl_Position.xyw;
	screenPosition.xy = (screenPosition.xy + screenPosition.z) * .5;

	vec4 viewPos = viewMatrix * vec4(worldPosition, 1.);
	vec2 horzRelativePos = worldPosition.xy - viewOrigin.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance).xyz;

	viewPosition = viewPos.xyz;


	PrepareForShadow(worldPosition, vec3(0., 0., -1.));
}

