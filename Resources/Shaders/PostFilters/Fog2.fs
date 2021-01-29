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

uniform vec3 viewOrigin;
uniform vec3 fogColor;
uniform float fogDistance;
uniform mat4 viewProjectionMatrixInv;

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

	// ---------------------------------------------------------------------
	// Calculate the in-scattering radiance (the amount of incoming light scattered
	// toward the camera).

	float weightSum = 0.0;
	const int numSamples = 16;

	// Shadow map sampling + AO
	float fogColorFactor = 0.0;

	// Shadow map sampling
	vec3 currentShadowPosition = transformToShadow(viewOrigin);
	vec3 shadowPositionDelta = transformToShadow(viewcentricWorldPosition.xyz / float(numSamples));

	for (int i = 0; i < numSamples; ++i) {
		// Shadows closer to the camera should be more visible
		float weight = 1.0 - float(i) / float(numSamples) * sqrt(voxlapDistanceSq) / fogDistance;
		weightSum += weight;

		// Shadow map sampling
		float val = texture2D(shadowMapTexture, currentShadowPosition.xy).w;
		val = step(currentShadowPosition.z, val);
		fogColorFactor += val * weight;

		currentShadowPosition += shadowPositionDelta;
	}

	// Rescale the in-scattering term according to the desired fog density
	float scale = goalFogFactor / (weightSum + 1.0e-4);
	fogColorFactor *= scale;

	// ---------------------------------------------------------------------

	gl_FragColor = texture2D(colorTexture, texCoord);
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
#endif

	gl_FragColor.xyz += goalFogFactor * fogColor * fogColorFactor;

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}
