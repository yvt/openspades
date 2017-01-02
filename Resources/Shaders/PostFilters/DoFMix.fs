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


uniform sampler2D mainTexture;
uniform sampler2D blurTexture1;
uniform sampler2D blurTexture2;
uniform sampler2D cocTexture;
uniform bool blurredOnly;

varying vec2 texCoord;

vec4 doGamma(vec4 col) {
#if !LINEAR_FRAMEBUFFER
	col.xyz *= col.xyz;
#endif
	return col;
}

void main() {

	float coc = texture2D(cocTexture, texCoord).x;

	vec4 a = doGamma(texture2D(mainTexture, texCoord));
	vec4 b = doGamma(texture2D(blurTexture1, texCoord));
	b += doGamma(texture2D(blurTexture2, texCoord)) * 2.;
	b *= (1. / 3.);

	float per = min(1., coc * 5.);
	vec4 v = blurredOnly ? b : mix(a, b, per);

#if !LINEAR_FRAMEBUFFER
	v.xyz = sqrt(v.xyz);
#endif

	gl_FragColor = v;
}

