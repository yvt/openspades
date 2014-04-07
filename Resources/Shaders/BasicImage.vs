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



attribute vec2 positionAttribute;
attribute vec4 colorAttribute;
attribute vec2 textureCoordAttribute;

uniform vec2 invScreenSizeFactored;
uniform vec2 invTextureSize;

varying vec4 color;
varying vec2 texCoord;

void main() {
	
	vec2 pos = positionAttribute;
	
	pos = pos * invScreenSizeFactored + vec2(-1., 1.);
	
	/*
	pos /= screenSize;
	pos = pos * 2. - 1.;
	pos.y = -pos.y; */
	
	gl_Position = vec4(pos, 0.5, 1.);
	
	color = colorAttribute;
	texCoord = textureCoordAttribute * invTextureSize;
}

