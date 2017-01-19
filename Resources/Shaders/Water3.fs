#extension GL_EXT_texture_array : require
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
varying vec2 worldPositionOriginal;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2D mainTexture;
uniform sampler2DArray waveTextureArray;
uniform sampler2D mirrorTexture;
uniform sampler2D mirrorDepthTexture;
uniform mat4 viewMatrix;
uniform vec3 fogColor;
uniform vec3 skyColor;
uniform vec2 zNearFar;
uniform vec4 fovTan;
uniform vec4 waterPlane;
uniform vec3 viewOrigin;

uniform vec2 displaceScale;

vec3 EvaluateSunLight();
vec3 EvaluateAmbientLight(float detailAmbientOcclusion);
float GGXDistribution(float m, float dotHalf);

float decodeDepth(float w, float near, float far){
	return far * near / mix(far, near, w);
}

float encodeDepth(float z, float near, float far) {
	// FN/(F(1-w) + Nw) = z
	// FN = z(w(N-F) + F)
	// FN = zw(N-F) + Fz
	// w = F(N - z) / z(N - F)
	return far * (near + z) / (z * (far - near));
}

float depthAt(vec2 pt){
	float w = texture2D(depthTexture, pt).x;
	return decodeDepth(w, zNearFar.x, zNearFar.y);
}

void main() {
	vec3 worldPositionFromOrigin = worldPosition - viewOrigin;
	vec4 waveCoord = worldPositionOriginal.xyxy * vec4(vec2(0.04),
											   vec2(0.08704))
	+ vec4(0., 0., 0.754, 0.1315);
	vec2 waveCoord2 = worldPositionOriginal.xy * 0.00844 + vec2(.154, .7315);

	// evaluate waveform (normal vector)
	vec3 wave = texture2DArray(waveTextureArray, vec3(waveCoord.xy, 0.0)).xyz;
	wave = mix(vec3(-0.0025), vec3(0.0025), wave);
	wave.xy *= 0.04 * 1.8;

	// detail
	vec2 wave2 = texture2DArray(waveTextureArray, vec3(waveCoord.zw, 1.0)).xy;
	wave2 = mix(vec2(-0.0025), vec2(0.0025), wave2);
	wave2.xy *= 0.08704 * 1.2;
	wave.xy += wave2;

	// rough
	wave2 = texture2DArray(waveTextureArray, vec3(waveCoord2.xy, 2.0)).xy;
	wave2 = mix(vec2(-0.0025), vec2(0.0025), wave2);
	wave2.xy *= 0.00844 * 2.5;
	wave.xy += wave2;

	wave.z = (1. / 256.) / (4.); // (negated normal vector!)
	wave.xyz = normalize(wave.xyz);

	vec2 origScrPos = screenPosition.xy / screenPosition.z;
	vec2 scrPos = origScrPos;

	/* ------- Refraction -------- */

	// Compute the line segment for refraction ray tracing
	vec3 normalVS = (viewMatrix * vec4(-wave, 0.)).xyz;
	vec3 refractedVS = refract(normalize(viewPosition.xyz), normalVS, 1.0 / 1.5);
	vec3 refractTargetVS = viewPosition + refractedVS;
	if (refractTargetVS.z > -0.001) {
		refractTargetVS = mix(viewPosition, refractedVS, (-0.001 - viewPosition.z) / (refractedVS.z - viewPosition.z));
	}
	vec3 refractTargetNDC = vec3(
		refractTargetVS.xy / refractTargetVS.z / fovTan.xy,
		encodeDepth(refractTargetVS.z, zNearFar.x, zNearFar.y));

	float scale = 1. / viewPosition.z;
	vec2 disp = wave.xy * 0.1;
	scrPos += disp * scale * displaceScale  * 4.;

	vec2 refractTargetSS = refractTargetNDC.xy * vec2(-0.5, 0.5) + 0.5;

	// Screen-space ray tracing
	float origDepth = gl_FragCoord.z;
	vec2 targetScrPos = refractTargetSS;
	float targetDepth = refractTargetNDC.z;
	float depth;
	float dither = fract(dot(fract(gl_FragCoord.xy * 0.5), vec2(0.5)));
	for (float i = dither / 16.0; i <= 1.0; i += 1.0 / 16.0) {
		float rayDepth = mix(origDepth, targetDepth, i);
		refractTargetSS = mix(origScrPos, targetScrPos, i);
		depth = texture2D(depthTexture, refractTargetSS).x;
		if (depth < rayDepth && // ray intersects the object
			depth > rayDepth - 0.1) { // (perhaps ray's actually going behind the object!)
			i = max(0.0, i - 1.0 / 16.0);
			refractTargetSS = mix(origScrPos, targetScrPos, i);
			depth = texture2D(depthTexture, refractTargetSS).x;
			break;
		}
	}

	// convert to linear Z
	depth = decodeDepth(depth, zNearFar.x, zNearFar.y);

	// make sure the sampled point is above the water plane.
	// convert to view coord
	vec3 sampledViewCoord = vec3(mix(fovTan.zw, fovTan.xy, refractTargetSS), 1.) * -depth;
	float planeDistance = dot(vec4(sampledViewCoord, 1.), waterPlane);
 	if(planeDistance < 0.0){
		// reset!
		// original pos must be in the water.
		refractTargetSS = origScrPos;
		depth = depthAt(refractTargetSS);
		if(depth + viewPosition.z < 0.){
			// if the pixel is obscured by a object,
			// this fragment of water is not visible
			//discard; done by early-Z test
		}

		sampledViewCoord = vec3(mix(fovTan.zw, fovTan.xy, refractTargetSS), 1.) * -depth;
	}

	float envelope = min(distance(viewPosition * vec3(-1., 1., 1.), sampledViewCoord) * 0.8, 1.);
	envelope = 1. - (1. - envelope) * (1. - envelope);

	// Blend the water color
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
	waterColor *= EvaluateSunLight() + EvaluateAmbientLight(1.);

	// underwater object color
	gl_FragColor = texture2D(screenTexture, refractTargetSS);
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

	/* ------- Reflection -------- */

	// Compute the line segment for refraction ray tracing
	vec3 reflectedVS = reflect(normalize(viewPosition.xyz), normalVS);
	reflectedVS = reflect(reflectedVS, waterPlane.xyz); // reflection's Z position is inverted
	vec3 reflectTargetVS = viewPosition + reflectedVS * (abs(viewPosition.z) + 1.);
	if (reflectTargetVS.z > -0.001) {
		reflectTargetVS = mix(viewPosition, reflectedVS, (-0.001 - viewPosition.z) / (reflectedVS.z - viewPosition.z));
	}
	vec3 reflectTargetNDC = vec3(
		reflectTargetVS.xy / reflectTargetVS.z / fovTan.xy,
		encodeDepth(reflectTargetVS.z, zNearFar.x, zNearFar.y));

	vec2 reflectTargetSS = reflectTargetNDC.xy * vec2(-0.5, 0.5) + 0.5;

	// Screen-space ray tracing
	targetScrPos = reflectTargetSS;
	targetDepth = reflectTargetNDC.z;
	for (float i = dither / 16.0; i <= 1.0; i += 1.0 / 16.0) {
		float rayDepth = mix(origDepth, targetDepth, i);
		reflectTargetSS = mix(origScrPos, targetScrPos, i);
		depth = texture2D(mirrorDepthTexture, reflectTargetSS).x;
		if (depth < rayDepth && // ray intersects the object
			depth > rayDepth - 0.1) { // (perhaps ray's actually going behind the object!)
			i = max(0.0, i - 1.0 / 16.0);
			reflectTargetSS = mix(origScrPos, targetScrPos, i);
			depth = texture2D(mirrorDepthTexture, reflectTargetSS).x;
			break;
		}
	}

	// convert to linear Z
	bool reflectedSky = depth > 0.99999;
	depth = decodeDepth(depth, zNearFar.x, zNearFar.y);

	// make sure the reflection is from the above the water plane
	sampledViewCoord = vec3(mix(fovTan.zw, fovTan.xy, reflectTargetSS), 1.) * -depth;
	planeDistance = dot(vec4(sampledViewCoord, 1.), waterPlane);
	bool validReflection = planeDistance > 0.0;

	vec3 reflected = texture2D(mirrorTexture, reflectTargetSS).xyz;

	if (!validReflection) {
		// The mirrored framebuffer isn't providing a valid reflected image.
		// Retry ray trace on the normal framebuffer

		// Compute the line segment for refraction ray tracing
		reflectedVS = reflect(normalize(viewPosition.xyz), normalVS);
		reflectTargetVS = viewPosition + reflectedVS * (abs(viewPosition.z) + 1.);
		if (reflectTargetVS.z > -0.001) {
			reflectTargetVS = mix(viewPosition, reflectedVS, (-0.001 - viewPosition.z) / (reflectedVS.z - viewPosition.z));
		}
		reflectTargetNDC = vec3(
			reflectTargetVS.xy / reflectTargetVS.z / fovTan.xy,
			encodeDepth(reflectTargetVS.z, zNearFar.x, zNearFar.y));

		reflectTargetSS = reflectTargetNDC.xy * vec2(-0.5, 0.5) + 0.5;

		// Screen-space ray tracing
		targetScrPos = reflectTargetSS;
		targetDepth = reflectTargetNDC.z;
		for (float i = dither / 32.0; i <= 1.0; i += 1.0 / 32.0) {
			float rayDepth = mix(origDepth, targetDepth, i);
			reflectTargetSS = mix(origScrPos, targetScrPos, i);
			depth = texture2D(depthTexture, reflectTargetSS).x;
			if (depth < rayDepth && // ray intersects the object
				depth > rayDepth - 0.1) { // (perhaps ray's actually going behind the object!)
				//i = max(0.0, i - 1.0 / 32.0);
				//reflectTargetSS = mix(origScrPos, targetScrPos, i);
				//depth = texture2D(depthTexture, reflectTargetSS).x;
				break;
			}
		}

		reflectedSky = depth > 0.99999;
		reflected = texture2D(screenTexture, reflectTargetSS).xyz;
	}

	vec3 ongoing = normalize(worldPositionFromOrigin);

    // bluring for far surface
	float lodBias = 1.0 / ongoing.z;
	float dispScaleByLod = min(1., ongoing.z * 0.5);
    lodBias = log2(lodBias);
    lodBias = clamp(lodBias, 0., 2.);

	// compute reflection color
	vec2 reflectionSS = origScrPos;
	disp.y = -abs(disp.y * 3.);
	reflectionSS -= disp * scale * displaceScale * 15.;

	vec3 refl = reflected;
#if !LINEAR_FRAMEBUFFER
	refl *= refl; // linearize
#endif

	// reflectivity
	vec3 sunlight = EvaluateSunLight();
	float reflective = dot(ongoing, wave.xyz);
	reflective = clamp(1. - reflective, 0., 1.);

    float orig_reflective = reflective;
	reflective *= reflective;
	reflective *= reflective;
    reflective = mix(reflective, orig_reflective * .6,
        clamp(lodBias * .13 - .13, 0., 1.));
	//reflective += .03;

	// reflection
#if USE_VOLUMETRIC_FOG
	// it's actually impossible for water reflection to cope with volumetric fog.
	// fade the water reflection so that we don't see sharp boundary of water
	refl *= att;
#endif
	gl_FragColor.xyz = mix(gl_FragColor.xyz,
						   refl,
						   reflective);


	/* ------- Specular Reflection -------- */

	// specular reflection
	if(dot(sunlight, vec3(1.)) > 0.0001 && reflectedSky){
		// can't use CockTorrance here -- CockTorrance's fresenel term
		// is hard-coded for higher roughness values
		vec3 halfVec = vec3(0., 1., 1.) + ongoing;
		halfVec = dot(halfVec, halfVec) < .00000000001 ? vec3(1., 0., 0.) : normalize(halfVec);
		float halfVecDot = max(dot(halfVec, wave), 0.00001);
		float m = 0.001 + 0.00015 / (abs(ongoing.z) + 0.0006); // roughness
		float spec = GGXDistribution(m, halfVecDot);

		// fresnel
		spec *= reflective;

		// geometric shadowing (Kelemen)
		float dot1 = dot(vec3(0., 1., 1.), wave);
		float dot2 = dot(ongoing, wave);
		float visibility = dot1 * dot2 / (halfVecDot * halfVecDot);
		spec *= max(0., visibility);

		// limit brightness (flickering specular reflection might cause seizure to some people)
		spec = min(spec, 120.);

		gl_FragColor.xyz += sunlight * spec * att;
	}

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif

	gl_FragColor.w = 1.;
}

