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

uniform vec3 fogColor;
uniform float fogDistance;
uniform mat4 viewProjectionMatrixInv;

varying vec2 texCoord;
varying vec4 viewcentricWorldPositionPartial;

float fogDensFunc(float time) {
	return time;
}

void main() {
	float localClipZ = texture2D(depthTexture, texCoord).x;
	vec4 viewcentricWorldPosition =
	  viewcentricWorldPositionPartial + viewProjectionMatrixInv * vec4(0.0, 0.0, localClipZ, 0.0);
	viewcentricWorldPosition.xyz /= viewcentricWorldPosition.w;

	// Clip the ray by the fog end distance.
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

	// TODO: Apply shadowing by volumetric ray marching

	gl_FragColor = texture2D(colorTexture, texCoord);
#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz *= gl_FragColor.xyz; // linearize
#endif

	gl_FragColor.xyz += goalFogFactor * fogColor;

#if !LINEAR_FRAMEBUFFER
	gl_FragColor.xyz = sqrt(gl_FragColor.xyz);
#endif
}
