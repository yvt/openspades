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


uniform sampler2D texture1;
uniform sampler2D texture2;

varying vec2 texCoord;

uniform vec3 mix1;
uniform vec3 mix2;

void main() {
	vec3 color1, color2;
	color1 = texture2D(texture1, texCoord).xyz;
	color2 = texture2D(texture2, texCoord).xyz;

	// as of 0.2.0, all colors are linearly encoded. There's no gamma correction here
	vec3 color = color1 * mix1 + color2 * mix2;

	gl_FragColor = vec4(color, 1.);
}

