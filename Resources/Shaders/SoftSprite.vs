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
uniform vec3 rightVector;
uniform vec3 upVector;
uniform vec3 frontVector;
uniform vec3 viewOriginVector;

uniform float fogDistance;

attribute vec4 positionAttribute;
attribute vec3 spritePosAttribute;
attribute vec4 colorAttribute;

varying vec4 color;
varying vec4 texCoord;
varying vec4 fogDensity;
varying vec4 depthRange;

void main() {
	vec3 center = positionAttribute.xyz;
	
	vec3 pos = center;
	float radius = positionAttribute.w;
	
	vec3 right = rightVector * radius;
	vec3 up = upVector * radius;
	
	float angle = spritePosAttribute.z;
	float c = cos(angle), s = sin(angle);
	vec2 sprP;
	sprP.x = dot(spritePosAttribute.xy, vec2(c, -s));
	sprP.y = dot(spritePosAttribute.xy, vec2(s, c));
	sprP *= radius;
	pos += right * sprP.x;
	pos += up * sprP.y;
	
	// move sprite to the front of the volume
	float centerDepth = dot(center - viewOriginVector, frontVector);
	depthRange.xy = vec2(centerDepth) + vec2(-1., 1.) * radius;
	depthRange.z = radius * 2.;
	
	// clip the volume by the near clip plane
	float frontDepth = depthRange.x;
	frontDepth = max(frontDepth, .3);
	/*if(frontDepth > depthRange.y) // go beyond near clip plane
		discard;*/ // cannot discard in vertex shader...
	frontDepth = min(frontDepth, depthRange.y);
	depthRange.w = frontDepth;
	
	pos += frontVector * (frontDepth - centerDepth);
	
	gl_Position = projectionViewMatrix * vec4(pos, 1.);
	
	
	color = colorAttribute;
	
	// sprite texture coord
	texCoord.xy = spritePosAttribute.xy * .5 + .5;
	
	// depth texture coord
	texCoord.zw = vec2(.5) + (gl_Position.xy / gl_Position.w) * .5;
	
	// fog.
	// cannot gamma correct because sprite may be
	// alpha-blended.
	vec4 viewPos = viewMatrix * vec4(pos,1.);
	float distance = length(viewPos.xyz) / fogDistance;
	distance = clamp(distance, 0., 1.);
	fogDensity = vec4(distance);
	fogDensity = pow(fogDensity, vec4(1., .9, .7, 1.));
	fogDensity *= fogDensity; // FIXME
}

