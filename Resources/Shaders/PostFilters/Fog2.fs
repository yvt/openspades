/*
 Copyright (c) 2021 yvt

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

uniform sampler2D colorTexture;
uniform sampler2D depthTexture;
uniform sampler2D shadowMapTexture;
uniform sampler2D ditherTexture;
uniform sampler3D ambientShadowTexture;
uniform sampler3D radiosityTexture;
uniform sampler2D noiseTexture;

const int NoiseTextureSize = 128;

uniform vec3 viewOrigin;
uniform vec3 sunlightScale;
uniform vec3 ambientScale;
uniform vec3 radiosityScale;
uniform float fogDistance;
uniform mat4 viewProjectionMatrixInv;
uniform vec2 ditherOffset;

varying vec2 texCoord;
varying vec4 viewcentricWorldPositionPartial;

/**
 * Transform the given world-space coordinates to the map-shadow space coordinates.
 * This function is linear.
 */
vec3 transformToShadow(vec3 v) {
	v.y -= v.z;
	v *= vec3(1. / 512., 1. / 512., 1. / 255.);
	return v;
}

vec3 DecodeRadiosityValue(vec3 val) {
	// reverse bias
	val *= 1023. / 1022.;
	val = (val * 2.) - 1.;
#if USE_RADIOSITY == 1
	// the low-precision radiosity texture uses a non-linear encoding
	val *= val * sign(val);
#endif
	return val;
}

void main() {
	float localClipZ = texture2D(depthTexture, texCoord).x;
	vec4 viewcentricWorldPosition =
	  viewcentricWorldPositionPartial + viewProjectionMatrixInv * vec4(0.0, 0.0, localClipZ, 0.0);
	viewcentricWorldPosition.xyz /= viewcentricWorldPosition.w;

	// Clip the ray by the VOXLAP fog end distance. (We don't want objects outside
	// the view distance to affect the fog shading.)
	float voxlapDistanceSq = dot(viewcentricWorldPosition.xy, viewcentricWorldPosition.xy);
	viewcentricWorldPosition /= max(sqrt(voxlapDistanceSq) / fogDistance, 1.0);
	voxlapDistanceSq = min(voxlapDistanceSq, fogDistance * fogDistance);

	// Calculate the supposed fog factor of the current pixel based on the
	// VOXLAP's cylindrical fog density model.
	float goalFogFactor = min(voxlapDistanceSq / (fogDistance * fogDistance), 1.0);
	if (localClipZ == 1.0) {
		// The sky should have the fog color.
		goalFogFactor = 1.0;
	}

	// OpenSpades' fog model uses a Rayleigh-scattering-style wavelength-
	// dependent fog density. (See `Shaders/Fog.vs`)
	vec3 goalFogFactorColor;
	{
		float weakenedDensity = 1. - goalFogFactor;
		weakenedDensity *= weakenedDensity;
		goalFogFactorColor =
		  mix(vec3(goalFogFactor), vec3(1. - weakenedDensity), vec3(0., 0.3, 1.0));
	}

	// ---------------------------------------------------------------------
	// Calculate the in-scattering radiance (the amount of incoming light scattered
	// toward the camera).

	float weightSum = 0.0;
	const int numSamples = 16;

	// Dithered sampling
	float dither2a =
	  texture2D(ditherTexture, gl_FragCoord.yx * 0.25 + ditherOffset + vec2(0.25, 0.0)).x * 15.0 /
	    16.0 -
	  0.5;
	float dither2b =
	  texture2D(ditherTexture, gl_FragCoord.yx * 0.25 + ditherOffset.yx + vec2(0.0, 0.5)).x * 15.0 /
	    16.0 -
	  0.5;

	// Add jitter based on a noise texture because the dither pattern solely
	// does not remove banding. The dither pattern is mostly useless for the
	// raymarching sampling position offset (`dither`). On the other hand, it's
	// more effective for soft shadowing.
	vec4 noiseValue = texture2D(noiseTexture, gl_FragCoord.xy / float(NoiseTextureSize));
	float dither = noiseValue.x;
	dither2a = (dither2a * 7.0 / 8.0) + (noiseValue.y - 0.5) / 8.0;
	dither2b = (dither2b * 7.0 / 8.0) + (noiseValue.z - 0.5) / 8.0;

	// Shadows closer to the camera should be more visible
	float weight = 1.0;
	float weightDelta = sqrt(voxlapDistanceSq) / fogDistance / float(numSamples);

	weight -= weightDelta * dither;

	// Shadow map sampling
	vec3 currentShadowPosition = transformToShadow(viewOrigin);
	vec3 shadowPositionDelta = transformToShadow(viewcentricWorldPosition.xyz / float(numSamples));

	currentShadowPosition += shadowPositionDelta * dither;
	currentShadowPosition +=
	  vec3(dither2a, dither2b, max(0.0, -dither2b)) / 512.0; // cheap soft shadowing

	float sunlightFactor = 0.0;

	// AO sampling
	vec3 currentRadiosityTextureCoord = (viewOrigin + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
	vec3 radiosityTextureCoordDelta =
	  viewcentricWorldPosition.xyz / float(numSamples) / vec3(512., 512., 64.);

	currentRadiosityTextureCoord += radiosityTextureCoordDelta * dither;

	float ambientFactor = 0.0;

	// Secondary diffuse reflection sampling
	vec3 currentAmbientShadowTextureCoord =
	  (viewOrigin + vec3(0.0, 0.0, 1.0)) / vec3(512., 512., 65.);
	vec3 ambientShadowTextureCoordDelta =
	  viewcentricWorldPosition.xyz / float(numSamples) / vec3(512., 512., 65.);
	vec3 radiosityFactor = vec3(0.0);
	float currentRadiosityCutoff = currentAmbientShadowTextureCoord.z * 10.0 + 1.0;
	float radiosityCutoffDelta = ambientShadowTextureCoordDelta.z * 10.0;

	currentAmbientShadowTextureCoord += ambientShadowTextureCoordDelta * dither;

	for (int i = 0; i < numSamples; ++i) {
		// Shadow map sampling
		float val = texture2D(shadowMapTexture, currentShadowPosition.xy).w;
		val = step(currentShadowPosition.z, val);
		sunlightFactor += val * weight;

		// AO sampling
		float aoFactor = texture3D(ambientShadowTexture, currentAmbientShadowTextureCoord).x;
		aoFactor = max(aoFactor, 0.); // for some reason, mainTexture value becomes negative(?)
		ambientFactor += aoFactor * weight;

		// Secondary diffuse reflection sampling
		//
		// Since the radiosity texture doesn't have the information above the `z = 0` plane,
		// gradually reduce the influence above the plane by multiplying `currentRadiosityCutoff`.
		vec3 radiosity =
		  DecodeRadiosityValue(texture3D(radiosityTexture, currentRadiosityTextureCoord).xyz);
		radiosityFactor += radiosity * (weight * clamp(currentRadiosityCutoff, 0.0, 1.0));

		currentShadowPosition += shadowPositionDelta;
		currentRadiosityTextureCoord += radiosityTextureCoordDelta;
		currentAmbientShadowTextureCoord += ambientShadowTextureCoordDelta;
		currentRadiosityCutoff += radiosityCutoffDelta;
		weightSum += weight;

		weight -= weightDelta;
	}

	vec3 sunlightFactorColor = sunlightFactor * sunlightScale;
	vec3 ambientFactorColor = ambientFactor * ambientScale;
	radiosityFactor *= radiosityScale;

	// Rescale the in-scattering term according to the desired fog density
	vec3 scale = goalFogFactorColor / (weightSum + 1.0e-4);
	radiosityFactor *= scale;
	sunlightFactorColor *= scale;
	ambientFactorColor *= scale;

	// ---------------------------------------------------------------------

	// add gradient
	vec3 sunDir = normalize(vec3(0., -1., -1.));
	float bright = dot(sunDir, normalize(viewcentricWorldPosition.xyz));
	sunlightFactorColor *= bright * 0.5 + 1.0;
	ambientFactorColor *= bright * 0.5 + 1.0;

	// ---------------------------------------------------------------------

	gl_FragColor = texture2D(colorTexture, texCoord);
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
#endif

	gl_FragColor.xyz += sunlightFactorColor + ambientFactorColor + radiosityFactor;

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}
