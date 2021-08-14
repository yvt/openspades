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
varying vec3 emissionColor;
varying vec2 ambientOcclusionCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;
varying float sunlightShading;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D detailTexture;
uniform vec3 fogColor;
uniform float modelOpacity;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

void main() {
	// color is linearized
	gl_FragColor = color;

	vec3 shading = vec3(sunlightShading);
	
	// FIXME: prepare for shadow?
	shading *= EvaluateSunLight();
	
	vec3 ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).xyz;
	shading += EvaluateAmbientLight(ao.x);
	
	gl_FragColor.xyz *= shading;

	gl_FragColor.xyz += emissionColor;

	//gl_FragColor.xyz *= texture2D(detailTexture, detailCoord).xyz * 2.;
	
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);
	
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif

	// Only valid in the ghost pass - Blending is disabled for most models
	gl_FragColor.w = modelOpacity;
}

