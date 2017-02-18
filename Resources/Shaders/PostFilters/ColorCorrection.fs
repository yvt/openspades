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

varying vec2 texCoord;

uniform float enhancement;
uniform float saturation;
uniform vec3 tint;

vec3 acesToneMapping(vec3 x)
{
	return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
	// Input is in the device color space
	gl_FragColor = texture2D(mainTexture, texCoord);

	gl_FragColor.xyz *= tint;

	vec3 gray = vec3(dot(gl_FragColor.xyz, vec3(1. / 3.)));
	gl_FragColor.xyz = mix(gray, gl_FragColor.xyz, saturation);

#if USE_HDR
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
	gl_FragColor.xyz = acesToneMapping(gl_FragColor.xyz * 0.8);
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz); // delinearize
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   smoothstep(0., 1., gl_FragColor.xyz),
						   enhancement);
#else
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   smoothstep(0., 1., gl_FragColor.xyz),
						   enhancement);
#endif

	gl_FragColor.w = 1.;

}

