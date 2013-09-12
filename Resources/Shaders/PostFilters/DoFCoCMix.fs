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


uniform sampler2D cocTexture;
uniform sampler2D cocBlurTexture;

varying vec2 texCoord;


void main() {
	
	float coc = texture2D(cocTexture, texCoord).x;
	float cocBlur = texture2D(cocBlurTexture, texCoord).x;

	float op = 2. * max(cocBlur, coc) - coc;
	op = max(op, coc);
	gl_FragColor = vec4(op);
	
}

