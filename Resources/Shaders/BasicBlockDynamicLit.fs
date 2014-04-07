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



varying vec4 color;
varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D detailTexture;
uniform vec3 fogColor;

vec3 EvaluateDynamicLightNoBump();

void main() {
	// color is linearized
	gl_FragColor = vec4(color.xyz, 1.);
	
	vec3 shading = EvaluateDynamicLightNoBump();
	
	gl_FragColor.xyz *= shading;
	
	// fog fade
	gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(0.), fogDensity);
	
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}

