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
uniform sampler2D cocTexture;

varying vec2 texCoord;
uniform vec2 offset;

vec4 doGamma(vec4 col) {
	col.xyz *= col.xyz;
	return col;
}

void main() {
	
	float coc = texture2D(cocTexture, texCoord).x;
	vec4 v = vec4(0.);
	
	vec4 offsets = vec4(0., 0.25, 0.5, 0.75) * coc;
	
	v += doGamma(texture2D(texture, texCoord));
	v += doGamma(texture2D(texture, texCoord + offset * offsets.y));
	v += doGamma(texture2D(texture, texCoord + offset * offsets.z));
	v += doGamma(texture2D(texture, texCoord + offset * offsets.w));

	v *= 0.25;
	v.xyz = sqrt(v.xyz);
	
	gl_FragColor = v;
}

