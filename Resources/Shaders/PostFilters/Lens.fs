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


uniform sampler2D mainTexture;

varying vec3 angleTan;
varying vec4 texCoord1;
varying vec4 texCoord2;
varying vec4 texCoord3;
varying vec2 texCoord4;

// linearize gamma
vec3 linearlize(vec3 col){
#if !LINEAR_FRAMEBUFFER
	return col * col;
#else
	return col;
#endif
}

void main() {
	vec3 sum = vec3(0.), val;
	
#if 1
	// accurate color abberation
	// FIXME: handle LINEAR_FRAMEBUFFER case
	val = vec3(0.0, 0.2, 1.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord1.xy).xyz);
	gl_FragColor.xyz = val;
	
	val = vec3(0.0, 0.5, 1.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord1.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(0.0, 1.0, 0.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord2.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.5, 0.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord2.zw).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.4, 0.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord3.xy).xyz);
	gl_FragColor.xyz += val;
	
	val = vec3(1.0, 0.3, 1.0);
	sum += val;
	val *= linearlize(texture2D(mainTexture, texCoord3.zw).xyz);
	gl_FragColor.xyz += val;
	
	gl_FragColor.xyz *= 1. / sum;
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
	
#elif 1
	// faster!
	gl_FragColor.x = texture2D(mainTexture, texCoord4.xy).x;
	gl_FragColor.y = texture2D(mainTexture, texCoord2.xy).y;
	gl_FragColor.z = texture2D(mainTexture, texCoord1.xy).z;
#else
	// no color abberation effect
	gl_FragColor = texture2D(mainTexture, texCoord4);
	gl_FragColor.w = 1.;
#endif
	
	// calc brightness (cos^4)
	// note that this is gamma corrected
	float tanValue = length(angleTan.xy);
	float brightness = 1. / (1. + tanValue * tanValue);
	brightness *= angleTan.z;
#if LINEAR_FRAMEBUFFER
	brightness *= brightness;
#endif
#if USE_HDR
	brightness = mix(brightness, 1., 0.7); // weaken
#else
	brightness = mix(brightness, 1., 0.9); // weaken
#endif
	
	gl_FragColor.xyz *= brightness;
	
	gl_FragColor.w = 1.;
}

