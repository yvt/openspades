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

uniform vec4 u_DrawRange;

varying vec2 v_TexCoord;
varying vec2 v_ModulationTexCoord;

void main() {
    gl_Position.xy = mix(u_DrawRange.xy, u_DrawRange.zw,
                         a_Position.xy);
    gl_Position.z = 0.5;
    gl_Position.w = 1.;

    v_TexCoord = mix(vec2(0.), vec2(1.), a_Position.xy);
    v_ModulationTexCoord = gl_Position.xy * 0.5 + vec2(0.5, 0.5);
}
