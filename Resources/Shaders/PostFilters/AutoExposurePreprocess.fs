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


uniform sampler2D mainTexture;

varying vec2 texCoord;

void main() {
	// linear RGB
	vec3 color = texture2D(mainTexture, texCoord).xyz;

	// desaturate
	float brightness = max(max(color.x, color.y), color.z);

	// remove NaN and Infinity
	if (!(brightness >= 0. && brightness <= 16.)) {
		brightness = 0.05;
	}

	// lower bound
	brightness = max(0.05, brightness);

	// uppr bound
	brightness = min(1.3, brightness);

	// raise to the n-th power to reduce overbright
	brightness *= brightness;
	brightness *= brightness;

	gl_FragColor = vec4(brightness, brightness, brightness, 1.);
}

