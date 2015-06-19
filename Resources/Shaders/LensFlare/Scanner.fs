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



uniform sampler2DShadow depthTexture;

varying vec3 scanPos;
varying vec2 circlePos;

uniform float radius;

void main() {
	float val = shadow2D(depthTexture, scanPos).x;
	
	// circle trim
	float rad = length(circlePos);
	rad *= radius;
	rad = clamp(radius - 1. - rad, 0., 1.);
	val *= rad;
	
	gl_FragColor = vec4(vec3(val), 1.);
}

