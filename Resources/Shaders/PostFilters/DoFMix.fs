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


uniform sampler2D texture;
uniform sampler2D blurTexture1;
uniform sampler2D blurTexture2;
uniform sampler2D cocTexture;

varying vec2 texCoord;

vec4 doGamma(vec4 col) {
	col.xyz *= col.xyz;
	return col;
}

void main() {
	
	float coc = texture2D(cocTexture, texCoord).x;
	
	vec4 a = doGamma(texture2D(texture, texCoord));
	vec4 b = doGamma(texture2D(blurTexture1, texCoord));
	b += doGamma(texture2D(blurTexture2, texCoord));
	b *= 0.5;
	
	float per = clamp(0., 1., coc * 5.);
	vec4 v = mix(a, b, per);
	v.xyz = sqrt(v.xyz);
	
	gl_FragColor = v;
}

