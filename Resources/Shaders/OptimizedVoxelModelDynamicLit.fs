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



varying vec2 textureCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;

uniform sampler2D modelTexture;
uniform vec3 customColor;
//uniform sampler2D detailTexture;

vec3 EvaluateDynamicLightNoBump();

void main() {
	
	vec4 texData = texture2D(modelTexture, textureCoord.xy);
	
	// model color
	gl_FragColor = vec4(texData.xyz, 1.);
	if(dot(gl_FragColor.xyz, vec3(1.)) < 0.0001){
		gl_FragColor.xyz = customColor;
	}

	bool isEmissive = texData.w == 1.0;
	if (isEmissive) {
		discard;
	}

	// linearize
	gl_FragColor.xyz *= gl_FragColor.xyz;
	
	// lighting
	vec3 shading = EvaluateDynamicLightNoBump();
	gl_FragColor.xyz *= shading;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(0.), fogDensity);
	
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}

