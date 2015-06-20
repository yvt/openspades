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

varying vec2 texCoord;

uniform float enhancement;
uniform float saturation;
uniform vec3 tint;

void main() {
	
	gl_FragColor = texture2D(texture, texCoord);
	
	gl_FragColor.xyz *= tint;
	
	vec3 gray = vec3(dot(gl_FragColor.xyz, vec3(1. / 3.)));
	gl_FragColor.xyz = mix(gray, gl_FragColor.xyz, saturation);

#if USE_HDR
	
	vec3 filtered = smoothstep(0., 1., gl_FragColor.xyz);
	vec3 temporal = gl_FragColor.xyz - 0.8;
	filtered -= exp2(-7. * temporal * temporal) * 0.2;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   max(filtered, vec3(0.)),
						   enhancement);
#else
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   smoothstep(0., 1., gl_FragColor.xyz),
						   enhancement);
#endif

	gl_FragColor.w = 1.;

}

