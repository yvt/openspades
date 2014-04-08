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



attribute vec2 positionAttribute;
attribute vec4 colorAttribute;

uniform vec2 fov;

varying vec3 angleTan;
varying vec4 texCoord1;
varying vec4 texCoord2;
varying vec4 texCoord3;
varying vec2 texCoord4;

void main() {
	
	vec2 pos = positionAttribute;
	
	vec2 scrPos = pos * 2. - 1.;
	
	gl_Position = vec4(scrPos, 0.5, 1.);
	
	// texture coords
	vec2 startCoord = pos;
	vec2 diff = vec2(.5) - startCoord;
	diff *= 0.008;
	
	startCoord += diff * .7;
	diff = -diff;
	
	texCoord1 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.), vec2(.1));
	texCoord2 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.2), vec2(.3));
	texCoord3 = startCoord.xyxy + diff.xyxy * vec4(vec2(0.4), vec2(.5));
	texCoord4 = pos.xy;
	
	// view angle
	angleTan.xy = scrPos * fov;
	
	// angleTan.z = brightness scale (to make brightness gain at corner (most darkest) become 1)
	//            = 1 / cos(fovDiag)
	angleTan.z = length(fov) * length(fov) + 1.;
	
	// weaken the brightness adjust so that saturation doesn't occur
#if USE_HDR
	angleTan.z = mix(angleTan.z, 1., 0.6);
#else
	// saturation is likely to occur in LDR, so weaken more
	angleTan.z = mix(angleTan.z, 1., 0.9);
#endif
}

