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



varying vec4 textureCoord;
//varying vec2 detailCoord;
varying vec3 fogDensity;
varying float flatShading;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D modelTexture;
uniform vec3 fogColor;
uniform vec3 customColor;
uniform float modelOpacity;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

void main() {
	vec4 texData = texture2D(modelTexture, textureCoord.xy);

	// model color
	gl_FragColor = vec4(texData.xyz, 1.);
	if(dot(gl_FragColor.xyz, vec3(1.)) < 0.0001){
		gl_FragColor.xyz = customColor;
	}

	// ambient occlusion
	float aoID = texData.w * (255. / 256.);

	float aoY = aoID * 16.;
	float aoX = fract(aoY);
	aoY = floor(aoY) / 16.;

	vec2 ambientOcclusionCoord = vec2(aoX, aoY);
	ambientOcclusionCoord += fract(textureCoord.zw) *
		(15. / 256.);
	ambientOcclusionCoord += .5 / 256.;

	// Emissive material flag is encoded in AOID
	bool isEmissive = texData.w == 1.0;

	// linearize
	gl_FragColor.xyz *= gl_FragColor.xyz;

	// shading
	vec3 shading = vec3(flatShading);

	shading *= EvaluateSunLight();

	vec3 ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).xyz;
	shading += EvaluateAmbientLight(ao.x);

	if (!isEmissive) {
		gl_FragColor.xyz *= shading;
	}

	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif

	// Only valid in the ghost pass - Blending is disabled for most models
	gl_FragColor.w = modelOpacity;
}

