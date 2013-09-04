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
varying vec2 ambientOcclusionCoord;
varying vec3 fogDensity;

varying vec3 viewSpaceCoord;
varying vec3 viewSpaceNormal;
uniform vec3 viewSpaceLight;

uniform sampler2D ambientOcclusionTexture;
uniform sampler2D detailTexture;
uniform vec3 fogColor;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);
//void VisibilityOfSunLight_Model_Debug();

float OrenNayar(float sigma, float dotLight, float dotEye);
float CockTorrance(vec3 eyeVec, vec3 lightVec, vec3 normal);

void main() {
	// color is linear
	gl_FragColor = vec4(color.xyz, 1.);
	
	vec3 shading = vec3(OrenNayar(.8, color.w,
							 -dot(viewSpaceNormal, normalize(viewSpaceCoord))));
	vec3 sunLight = EvaluateSunLight();
	shading *= sunLight;
	
	float ao = texture2D(ambientOcclusionTexture, ambientOcclusionCoord).x;
	
	shading += EvaluateAmbientLight(ao);
	
	// apply diffuse shading
	gl_FragColor.xyz *= shading;
	
	// specular shading
	if(color.w > .1 && dot(sunLight, vec3(1.)) > 0.001){
		vec3 specularColor = sunLight;
		gl_FragColor.xyz += specularColor * CockTorrance(-normalize(viewSpaceCoord),
													viewSpaceLight,
													viewSpaceNormal);
	}
	
	// apply fog
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColor, fogDensity);
	
	// gamma correct
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
}

