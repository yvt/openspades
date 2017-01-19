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
uniform sampler2D cocTexture;

varying vec2 texCoord;
uniform vec2 offset;

void main() {

	float coc = texture2D(cocTexture, texCoord).x;
	vec4 v = vec4(0.);

	vec4 offsets = vec4(0., 0.25, 0.5, 0.75) * coc;
	vec4 offsets2 = offsets + coc * 0.125;

	v += texture2D(mainTexture, texCoord);
	v += texture2D(mainTexture, texCoord + offset * offsets.y);
	v += texture2D(mainTexture, texCoord + offset * offsets.z);
	v += texture2D(mainTexture, texCoord + offset * offsets.w);
	v += texture2D(mainTexture, texCoord + offset * offsets2.x);
	v += texture2D(mainTexture, texCoord + offset * offsets2.y);
	v += texture2D(mainTexture, texCoord + offset * offsets2.z);
	v += texture2D(mainTexture, texCoord + offset * offsets2.w);
	v *= 0.125;

	gl_FragColor = v;
}

