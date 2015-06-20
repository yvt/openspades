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

uniform sampler2D texture;

varying vec4 color;

void main() {
	float brightness = texture2D(texture, vec2(0., 0.)).x;

	// makes sure we have no NaN and Infinity (where's isfinite?)
	if (!(brightness >= 0. && brightness <= 16.)) {
		gl_FragColor = vec4(0.);
		return;
	}

	// reverse "raise to the 2nd power"
	brightness = sqrt(brightness);
	brightness = sqrt(brightness);

	// weaken the effect
	brightness = mix(brightness, 1., 0.2);

	gl_FragColor.xyz = vec3(.8 / brightness);
	gl_FragColor.w = color.w;
}

