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

attribute vec2 positionAttribute;
attribute vec4 colorAttribute;

uniform mat4 viewProjectionMatrixInv;

varying vec2 texCoord;
varying vec4 viewcentricWorldPositionPartial;

void main() {
	vec2 pos = positionAttribute;

	vec2 scrPos = pos * 2. - 1.;
	gl_Position = vec4(scrPos, 0.5, 1.);

	texCoord = pos;

	viewcentricWorldPositionPartial = viewProjectionMatrixInv * vec4(pos, 0.0, 1.0);
}
