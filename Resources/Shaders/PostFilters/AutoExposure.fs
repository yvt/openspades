/*
 Copyright (c) 2015 yvt

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

// This shader computes the gain of the auto exposure.

uniform sampler2D mainTexture;

uniform float minGain;
uniform float maxGain;

varying vec4 color;

void main() {
	float brightness = texture2D(mainTexture, vec2(0.5, 0.5)).x;

	// reverse "raise to the n-th power"
	brightness = sqrt(brightness);
	brightness = sqrt(brightness);

	// weaken the effect
	// brightness = mix(brightness, 1., 0.05);

	gl_FragColor.xyz = vec3(clamp(.6 / brightness, minGain, maxGain));
	gl_FragColor.w = color.w;
}

