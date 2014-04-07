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


uniform sampler2D texture;

varying vec3 angleTan;
varying vec4 texCoord1;
varying vec4 texCoord2;
varying vec4 texCoord3;
varying vec2 texCoord4;

// linearize gamma
vec3 filter(vec3 col){
	return col * col;
}

void main() {
	vec3 sum = vec3(0.), val;
	
#if 0
	// accurate color abberation
	// FIXME: handle LINEAR_FRAMEBUFFER case
	val = vec3(0.0, 0.2, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord1.xy).xyz);
	gl_FragColor.xyz = val;
	
	val = vec3(0.0, 0.5, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord1.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(0.0, 1.0, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord2.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.5, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord2.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.4, 0.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord3.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.3, 1.0);
	sum += val;
	val *= filter(texture2D(texture, texCoord3.zw).xyz);
	gl_FragColor.xyz += val;
	
	gl_FragColor.xyz *= 1. / sum;
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#elif 1
	// faster!
	gl_FragColor.x = texture2D(texture, texCoord2.xy).x;
	gl_FragColor.y = texture2D(texture, texCoord3.xy).y;
	gl_FragColor.z = texture2D(texture, texCoord4.xy).z;
#else
	// no color abberation effect
	gl_FragColor = texture2D(texture, texCoord4);
	gl_FragColor.w = 1.;
#endif
	
	// calc brightness (cos^4)
	// note that this is gamma corrected
	float tanValue = length(angleTan.xy);
	float brightness = 1. / (1. + tanValue * tanValue);
	brightness *= angleTan.z;
	brightness = mix(brightness, 1., 0.5); // weaken
	
	gl_FragColor.xyz *= brightness;
	
	gl_FragColor.w = 1.;
}

