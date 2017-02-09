/*
 Copyright (c) 2017 yvt

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

attribute vec2 a_Position;
attribute vec4 a_Color;
attribute vec2 a_TextureCoordinate;

uniform vec2 u_InvScreenSizeFactor;
uniform vec2 u_InvTextureSize;

varying vec4 v_Color;
varying vec2 v_TextureCoordinate;

void main() {
    vec2 position = a_Position;
    position = position * u_InvScreenSizeFactor + vec2(-1., 1.);

    gl_Position = vec4(position, 0.5, 1.);

    v_Color = a_Color;
    v_TextureCoordinate = a_TextureCoordinate * u_InvTextureSize;
}
