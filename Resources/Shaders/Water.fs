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



//varying vec2 detailCoord;
varying vec3 fogDensity;
varying vec3 screenPosition;
varying vec3 viewPosition;
varying vec3 worldPosition;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D mainTexture;
uniform sampler2D waveTexture;
uniform vec3 fogColor;
uniform vec3 skyColor;
uniform vec2 zNearFar;
uniform vec4 fovTan;
uniform vec4 waterPlane;
uniform vec3 viewOrigin;

uniform vec2 displaceScale;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);

float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

void main() {
	vec3 worldPositionFromOrigin = worldPosition - viewOrigin;
	vec4 waveCoord = worldPosition.xyxy * vec4(vec2(0.08), vec2(0.15704))
	+ vec4(0., 0., 0.754, 0.1315);
	vec2 waveCoord2 = worldPosition.xy * 0.02344 + vec2(.154, .7315);
	
	// evaluate waveform
	vec3 wave = texture2D(waveTexture, waveCoord.xy).xyz;
	wave = mix(vec3(-1.), vec3(1.), wave);
	wave.xy *= 0.08 / 200.;
	
	// detail (Far Cry seems to use this technique)
	vec2 wave2 = texture2D(waveTexture, waveCoord.zw).xy;
	wave2 = mix(vec2(-1.), vec2(1.), wave2);
	wave2.xy *= 0.15704 / 200.;
	wave.xy += wave2;
	
	// rough
	wave2 = texture2D(waveTexture, waveCoord2.xy).xy;
	wave2 = mix(vec2(-1.), vec2(1.), wave2);
	wave2.xy *= 0.02344 / 200.;
	wave.xy += wave2;
	
	wave.z = (1. / 128.);
	wave.xyz = normalize(wave.xyz);
	
	vec2 origScrPos = screenPosition.xy / screenPosition.z;
	vec2 scrPos = origScrPos;
	
	float scale = 1. / viewPosition.z;
	vec2 disp = wave.xy * 0.1;
	scrPos += disp * scale * displaceScale  * 4.;
	
	// check envelope length.
	// if the displaced location points the out of the water,
	// reset to the original pos.
	float depth = depthAt(scrPos);
	
	// convert to view coord
	vec3 sampledViewCoord = vec3(mix(fovTan.zw, fovTan.xy, scrPos), 1.) * -depth;
	float planeDistance = dot(vec4(sampledViewCoord, 1.), waterPlane);
 	if(planeDistance < 0.){
		// reset!
		// original pos must be in the water.
		scrPos = origScrPos;
		depth = depthAt(scrPos);
		if(depth + viewPosition.z < 0.){
			// if the pixel is obscured by a object,
			// this fragment of water is not visible
			//discard; done by early-Z test
		}
	}else{
		depth = planeDistance / dot(waterPlane, vec4(0.,0.,1.,0.));
		depth = abs(depth);
		depth -= viewPosition.z;
	}
	
	float envelope = clamp((depth + viewPosition.z), 0., 1.);
	envelope = 1. - (1. - envelope) * (1. - envelope);
	
	// water color
	// TODO: correct integral
	vec2 waterCoord = worldPosition.xy;
	vec2 integralCoord = floor(waterCoord) + .5;
	vec2 blurDir = (worldPositionFromOrigin.xy);
	blurDir /= max(length(blurDir), 1.);
	vec2 blurDirSign = mix(vec2(-1.), vec2(1.), step(0., blurDir));
	vec2 startPos = (waterCoord - integralCoord) * blurDirSign;
	vec2 diffPos = blurDir * envelope * blurDirSign * .5 /*limit blur*/;
	vec2 subCoord = 1. - clamp((vec2(0.5) - startPos) / diffPos,
						  0., 1.);
	vec2 sampCoord = integralCoord + subCoord * blurDirSign;
	vec3 waterColor = texture2D(mainTexture, sampCoord / 512.).xyz;
	
	// underwater object color
	gl_FragColor = texture2D(screenTexture, scrPos);
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz; // screen color to linear
#endif
	
	// apply fog color to water color now.
	// note that fog is already applied to underwater object.
	waterColor = mix(waterColor, fogColor, fogDensity);
	
	// blend water color with the underwater object's color.
	gl_FragColor.xyz = mix(gl_FragColor.xyz, waterColor, envelope);
	
	// attenuation factor for addition blendings below
	vec3 att = 1. - fogDensity;
	
	// reflectivity
	vec3 sunlight = EvaluateSunLight();
	vec3 ongoing = normalize(worldPositionFromOrigin);
	float reflective = dot(ongoing, wave.xyz);
	reflective = clamp(1. - reflective, 0., 1.);
	reflective *= reflective;
	reflective *= reflective;
	reflective += .03;
	
	// fresnel refrection to sky
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   mix(skyColor * reflective * .6,
							   fogColor, fogDensity),
						   reflective);
	
	
	
	// specular reflection
	if(dot(sunlight, vec3(1.)) > 0.0001){
		vec3 refl = reflect(ongoing,
							wave.xyz);
		float spec = dot(refl, normalize(vec3(0., -1., -1.)));
		spec = max(spec, 0.);
		
		/*
		float sunVisRadius = cos(3.141592654 / 180. / 60. * 32.);
		
		spec = (spec - (1. - (1. - sunVisRadius) * 2.)) / ((1. - sunVisRadius) * 2.);
		float fw = fwidth(spec);
		spec = clamp(.5 + (spec - .5) / fw, 0., 1.);
		*/
		spec *= spec; // ^2
		spec *= spec; // ^4
		spec *= spec; // ^16
		spec *= spec; // ^32
		spec *= spec; // ^64
		spec *= spec; // ^128
		spec *= spec; // ^256
		spec *= spec; // ^512
		spec *= spec; // ^1024
		spec *= reflective;
		gl_FragColor.xyz += sunlight * spec * 1000. * att;
		
		
	}
	
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
	
	gl_FragColor.w = 1.;
}

