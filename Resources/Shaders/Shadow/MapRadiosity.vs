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


#if 0

/**** REAL-TIME RADIOSITY (SLOW) *****/

uniform sampler2D mapShadowTexture;
uniform vec3 eyeOrigin;
uniform vec3 eyeFront;
uniform vec3 fogColor;

varying vec3 radiosity;
varying vec3 ambientShadowTextureCoord;
varying vec3 ambientColor;

vec3 MapRadiosity_Sample(vec2 mapShadowCoord,
						 vec3 vertexCoord, vec3 normal) {
	vec4 val = texture2D(mapShadowTexture, mapShadowCoord.xy);
	
	// extract value
	vec3 color = val.xyz * 2.;
	vec3 bits = floor(color); // highest bit
	color = fract(color); // RGB7, actually [0,254/255]
	
	// to linear
	color *= 2.;
	color *= color;
	
	float depth = val.w * 255.;
	
	// compute world position
	vec3 wp = mapShadowCoord.xyy * vec3(512., 512., 0.);
	vec3 wn;
	//wp.xy = floor(wp.xy) + .5;
	if(bits.x < .5) {
		// z-plane
		wp.yz += depth;
		wn = vec3(0., 0., -1.);
	}else {
		// negative y-plane
		wp.yz += depth - fract(wp.y); // FIXME: maybe wrong (not checked)
		wn = vec3(0., -1., 0.);
	}
	
	float distance2 = dot(vertexCoord - wp, vertexCoord - wp);
	vec3 ret = color;
	
	if(dot(normal, wn) > .99){
		// same plane
		return vec3(0.);
	}
	
	if(distance2 > .0001) {
		ret *= max(dot(wp - vertexCoord, normal) + .3, 0.);
		ret *= max(dot(vertexCoord - wp, wn) + .3, 0.);
		ret /= distance2 + 0.1;
		ret /= distance2 + 1.;
	}else{
		// same position (corner?)
		ret *= 0.8;
		return vec3(0.);
		
	}
		
	return ret;
}

void PrepareForRadiosity_Map(vec3 vertexCoord, vec3 normal) {
	// copied from Map.fs
	
	vec3 mapShadowCoord = vertexCoord;
	mapShadowCoord.y -= mapShadowCoord.z;
	
	// texture value is normalized unsigned integer
	mapShadowCoord.z /= 255.;
	
	// texture coord is normalized
	// FIXME: variable texture size
	mapShadowCoord.xy /= 512.;
	
	vec3 samp = vec3(0.);//MapRadiosity_Sample(mapShadowCoord.xy,
									//vertexCoord, normal);
	
#define SAMPLE(x, y) \
	samp += MapRadiosity_Sample(mapShadowCoord.xy + vec2(x,y) / 512. * 0.9, \
								vertexCoord, normal)
	
	float detail = 2. - length(vertexCoord - eyeOrigin) * .2;
	detail = clamp(detail, 0., 1.);
	
	if(dot(vertexCoord - eyeOrigin, eyeFront) < -2.){
		detail = 0.;
	}
	
	if(detail > 0.){
		SAMPLE(-4., -4.);
		SAMPLE(-1., -2.);
		SAMPLE(0., -2.);
		SAMPLE(1., -2.);
		SAMPLE(4., -4.);
		
		SAMPLE(-2., -1.);
		SAMPLE(2., -1.);
		SAMPLE(-2., 0.);
		SAMPLE(2., 0.);
		SAMPLE(-2., 1.);
		SAMPLE(2., 1.);
		
		SAMPLE(-4., 4.);
		SAMPLE(-1., 2.);
		SAMPLE(0., 2.);
		SAMPLE(1., 2.);
		SAMPLE(4., 4.);
		
		samp = mix(vec3(0.15), samp * 2., detail);
	}else{
		samp = vec3(0.15);
	}
	
	detail = 5. - length(vertexCoord - eyeOrigin) * .2;
	detail = clamp(detail, 0., 1.);
	
	if(detail > 0.){
		SAMPLE(-1., -1.);
		SAMPLE(1., -1.);
		SAMPLE(-1., 1.);
		SAMPLE(1., 1.);
		
		samp = mix(vec3(0.4), samp, detail);
	}else{
		samp = vec3(0.4);
	}
		
	SAMPLE(0., -1.);
	SAMPLE(-1., 0.);
	SAMPLE(0., 0.);
	SAMPLE(1., 0.);
	SAMPLE(0., 1.);
	
	// TODO: variable map size?
	ambientShadowTextureCoord = (vertexCoord + vec3(0.5, 0.5, 1.5)) / vec3(512., 512., 65.);
	ambientColor = mix(fogColor, vec3(1.), 0.2) * 0.5;
	
	radiosity = samp * 0.28 + 0.13 * ambientColor;
}

#else

/**** CPU RADIOSITY (FASTER?) *****/

varying vec3 radiosityTextureCoord;
varying vec3 ambientShadowTextureCoord;
varying vec3 normalVarying;

void PrepareForRadiosity_Map(vec3 vertexCoord, vec3 normal) {
	
	radiosityTextureCoord = (vertexCoord + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
	ambientShadowTextureCoord = (vertexCoord + vec3(0.5, 0.5, 1.5)) / vec3(512., 512., 65.);
	
	normalVarying = normal;
}

#endif