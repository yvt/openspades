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

/*
 * This bi-cubic spline interpolation code is based on
 *
 * http://www.dannyruijters.nl/cubicinterpolation/
 * https://github.com/DannyRuijters/CubicInterpolationCUDA
 *
 * @license Copyright (c) 2008-2013, Danny Ruijters. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  *  Neither the name of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 */
void bspline_weights(vec2 fraction, out vec2 w0, out vec2 w1, out vec2 w2, out vec2 w3) {
	vec2 one_frac = 1.0 - fraction;
	vec2 squared = fraction * fraction;
	vec2 one_sqd = one_frac * one_frac;

	w0 = 1.0 / 6.0 * one_sqd * one_frac;
	w1 = 2.0 / 3.0 - 0.5 * squared * (2.0 - fraction);
	w2 = 2.0 / 3.0 - 0.5 * one_sqd * (2.0 - one_frac);
	w3 = 1.0 / 6.0 * squared * fraction;
}

vec3 cubicTex2D(sampler2D tex, vec2 coord, vec2 inverseTexSize) {
	// transform the coordinate from [0,extent] to [-0.5, extent-0.5]
	vec2 coord_grid = coord - 0.5;
	vec2 index = floor(coord_grid);
	vec2 fraction = coord_grid - index;
	vec2 w0, w1, w2, w3;
	bspline_weights(fraction, w0, w1, w2, w3);

	vec2 g0 = w0 + w1;
	vec2 g1 = w2 + w3;
	vec2 h0 =
	  (w1 / g0) - vec2(0.5) + index; // h0 = w1/g0 - 1, move from [-0.5, extent-0.5] to [0, extent]
	vec2 h1 =
	  (w3 / g1) + vec2(1.5) + index; // h1 = w3/g1 + 1, move from [-0.5, extent-0.5] to [0, extent]

	// fetch the four linear interpolations
	vec3 tex00 = texture2D(tex, vec2(h0.x, h0.y) * inverseTexSize).xyz;
	vec3 tex10 = texture2D(tex, vec2(h1.x, h0.y) * inverseTexSize).xyz;
	vec3 tex01 = texture2D(tex, vec2(h0.x, h1.y) * inverseTexSize).xyz;
	vec3 tex11 = texture2D(tex, vec2(h1.x, h1.y) * inverseTexSize).xyz;

	// weigh along the y-direction
	tex00 = g0.y * tex00 + g1.y * tex01;
	tex10 = g0.y * tex10 + g1.y * tex11;

	// weigh along the x-direction
	return (g0.x * tex00 + g1.x * tex10);
}

uniform sampler2D mainTexture;
uniform vec2 inverseVP;

varying vec2 texCoord;

void main() {
	gl_FragColor.xyz = cubicTex2D(mainTexture, texCoord, inverseVP);
	gl_FragColor.w = 1.0;
}
