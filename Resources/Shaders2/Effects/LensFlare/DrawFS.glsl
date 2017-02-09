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

uniform sampler2D u_VisibilityTexture;
uniform sampler2D u_ModulationTexture;
uniform sampler2D u_FlareTexture;

varying vec2 v_TexCoord;
varying vec2 v_ModulationTexCoord;

uniform vec3 u_Color;

void main() {
    float val = texture2D(u_VisibilityTexture, v_TexCoord).x;
    gl_FragColor = vec4(u_Color * val, 1.);
    gl_FragColor.xyz *= texture2D(u_FlareTexture, v_TexCoord).xyz;
    gl_FragColor.xyz *= texture2D(u_ModulationTexture, v_ModulationTexCoord).xyz;
}
