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
uniform vec3 viewOriginVector;

uniform float fogDistance;

attribute vec3 positionAttribute;
attribute vec2 texCoordAttribute;
attribute vec4 colorAttribute;

varying vec4 color;
varying vec2 texCoord;
varying vec4 fogDensity;

vec4 FogDensity(float poweredLength);

void main() {
	vec3 pos = positionAttribute.xyz;

	gl_Position = projectionViewMatrix * vec4(pos,1.);

	color = colorAttribute;

	texCoord = texCoordAttribute;

	// fog.
	// FIXME: cannot gamma correct because sprite may be
	// alpha-blended.
	vec4 viewPos = viewMatrix * vec4(pos,1.);
	vec2 horzRelativePos = pos.xy - viewOriginVector.xy;
	float horzDistance = dot(horzRelativePos, horzRelativePos);
	fogDensity = FogDensity(horzDistance);
}

