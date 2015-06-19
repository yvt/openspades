/*
 Copyright (c) 2013 OpenSpades Developers
 
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



uniform sampler2D visibilityTexture;
uniform sampler2D modulationTexture;
uniform sampler2D flareTexture;

varying vec2 texCoord;
varying vec2 modulationTexCoord;

uniform vec3 color;

void main() {
	float val = texture2D(visibilityTexture, texCoord).x;
	gl_FragColor = vec4(color * val, 1.);
	gl_FragColor.xyz *= texture2D(flareTexture, texCoord).xyz;
    gl_FragColor.xyz *= texture2D(modulationTexture, modulationTexCoord).xyz;
}

