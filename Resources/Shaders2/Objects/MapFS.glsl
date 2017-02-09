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

#pragma include <Shaders2/Common/LightingFS.h.glsl>

varying vec4 v_Color;
varying vec2 v_AmbientOcclusionCoord;

uniform sampler2D u_AmbientOcclusionTexture;

void main() {
    PrincipledBRDFParameter material;
    material.diffuseColor = v_Color.xyz;
    meterial.roughness = 0.8;

    float detailAmbientOcclusion = texture2D(u_AmbientOcclusionTexture, v_AmbientOcclusionCoord).x;

    gl_FragColor = EvaluateLighting(material, detailAmbientOcclusion);
}
