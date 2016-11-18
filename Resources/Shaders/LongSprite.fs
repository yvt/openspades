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

uniform vec3 fogColor;

varying vec4 color;
varying vec2 texCoord;
varying vec4 fogDensity;

void main() {
	gl_FragColor = texture2D(mainTexture, texCoord);
#if LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz;
#endif
	gl_FragColor.xyz *= gl_FragColor.w; // premultiplied alpha
	gl_FragColor *= color;
	
	vec4 fogColorPremuld = vec4(fogColor, 1.);
	fogColorPremuld *= gl_FragColor.w;
	gl_FragColor = mix(gl_FragColor, fogColorPremuld, fogDensity);
	
	if(dot(gl_FragColor, vec4(1.)) < .002)
		discard;
}

