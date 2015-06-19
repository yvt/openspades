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



attribute vec2 positionAttribute;

uniform vec4 scanRange;
uniform vec4 drawRange;
uniform float scanZ;

varying vec3 scanPos;
varying vec2 circlePos;

void main() {
	scanPos.xy = mix(scanRange.xy, scanRange.zw,
				  positionAttribute.xy);
	scanPos.z = scanZ;
	gl_Position.xy = mix(drawRange.xy, drawRange.zw,
						 positionAttribute.xy);
	gl_Position.z = 0.5;
	gl_Position.w = 1.;
	
	circlePos = mix(vec2(-1.), vec2(1.), positionAttribute.xy);
}

