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


uniform sampler2D depthTexture;
uniform sampler2D mainTexture;

uniform vec3 sRGBFogColor;
uniform vec2 zNearFar;

varying vec4 color;
varying vec3 emission;
varying vec4 texCoord;
varying vec4 fogDensity;
varying vec4 depthRange;
varying vec3 normal;
varying vec4 dlR, dlG, dlB;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

float decodeDepth(float w, float near, float far){
	return 1. /*far * near*/ / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

void main() {
	
	// get depth
	float depth = depthAt(texCoord.zw);
	
	if(depth < /*depthRange.x*/ depthRange.y){
		discard;
	}
	
	vec3 nNormal = normalize(normal);
	
	// calculate diffuse color
	vec4 computedColor = color;
	vec3 diffuse = EvaluateSunLight();
	vec3 sunDir = normalize(vec3(0., -1., -1.));
	diffuse *= dot(nNormal, sunDir) * 0.4 + 0.6;
	vec4 extNormal = vec4(nNormal, 1.);
	diffuse.x += dot(extNormal, dlR);
	diffuse.y += dot(extNormal, dlG);
	diffuse.z += dot(extNormal, dlB);
	diffuse += EvaluateAmbientLight(1.);
	diffuse = sqrt(diffuse);
	computedColor.xyz *= diffuse;
	
	computedColor.xyz += emission;
	
	gl_FragColor = texture2D(mainTexture, texCoord.xy);
#if LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz;
#endif
	gl_FragColor.xyz *= gl_FragColor.w; // premultiplied alpha
	gl_FragColor *= computedColor;
	
	vec3 fogColorPremuld = sRGBFogColor;
	fogColorPremuld *= gl_FragColor.w;
	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColorPremuld, fogDensity.xyz);
	
	
	//float soft = (depth - depthRange.x) / (depthRange.y - depthRange.w);
	float soft = depth * depthRange.z + depthRange.x;
	soft = smoothstep(0., 1., soft);
	gl_FragColor *= soft;
	
	gl_FragColor = max(gl_FragColor, 0.);
	
	if(dot(gl_FragColor, vec4(1.)) < .002)
		discard;
}

