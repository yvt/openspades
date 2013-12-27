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
uniform mat4 viewModelMatrix;
uniform vec3 viewOrigin;
uniform float fogDistance;

// [x, y]
attribute vec2 positionAttribute;

varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec3 worldPosition;

uniform sampler2D waveTexture1;
uniform sampler2D waveTexture2;
uniform sampler2D waveTexture3;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);

float DisplaceWater(vec2 worldPos){
	
	vec4 waveCoord = worldPos.xyxy * vec4(vec2(0.08), vec2(0.15704))
	+ vec4(0., 0., 0.754, 0.1315);
	
	vec2 waveCoord2 = worldPos.xy * 0.02344 + vec2(.154, .7315);
	
	
	vec4 wave = texture2DLod(waveTexture1, waveCoord.xy, 0.).xyzw;
	float disp = mix(-0.1, 0.1, wave.w) * 1.;
	
	vec4 wave2 = texture2DLod(waveTexture2, waveCoord.zw, 0.).xyzw;
	disp += mix(-0.1, 0.1, wave2.w) * 0.5;
	
	wave2 = texture2DLod(waveTexture3, waveCoord2.xy, 0.).xyzw;
	disp += mix(-0.1, 0.1, wave2.w) * 2.5;
	
	return disp * 4.;
}

void main() {
	
	vec4 vertexPos = vec4(positionAttribute.xy, 0., 1.);
	
	worldPosition = (modelMatrix * vertexPos).xyz;

	worldPosition.z += DisplaceWater(worldPosition.xy);
	
	gl_Position = projectionViewMatrix * vec4(worldPosition, 1.);
	screenPosition = gl_Position.xyw;
	screenPosition.xy = (screenPosition.xy + screenPosition.z) * .5;
		
	vec4 viewPos = viewModelMatrix * vertexPos;
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec3(distance);
	fogDensity = pow(fogDensity, vec3(1., .9, .7));
	fogDensity *= fogDensity;
	
	viewPosition = viewPos.xyz;
	
	
	PrepareForShadow((modelMatrix * vertexPos).xyz, vec3(0., 0., -1.));
}

