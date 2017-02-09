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

#pragma include <Shaders2/Objects/SolidCommonVS.h.glsl>

uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelNormalMatrix;
uniform vec3 u_ModelOrigin;
uniform vec3 u_CustomColor;

// [x, y, z, AO ID]
attribute vec4 a_Position;

// [u, v]
attribute vec2 a_AmbientOcclusionCoordinate;

// [R, G, B, diffuse]
attribute vec4 a_Color;

// [x, y, z]
attribute vec3 a_Normal;

varying vec2 v_AmbientOcclusionCoord;
varying vec3 v_Color;
//varying vec2 detailCoord;

void PrepareForShadow(vec3 worldOrigin, vec3 normal);
vec4 FogDensity(float poweredLength);

void main() {

    vec3 worldPosition = (u_ModelMatrix * vec4(a_Position.xyz + u_ModelOrigin, 1.)).xyz;
    vec3 worldNormal = (u_ModelNormalMatrix * vec4(a_Normal.xyz, 0.)).xyz;

    // Compute color
    v_Color = a_Color.xyz;

    if(dot(v_Color.xyz, vec3(1.)) < 0.0001){
        v_Color.xyz = u_CustomColor;
    }

    // linearize
    v_Color.xyz *= v_Color.xyz;

    // ambient occlusion
    float aoID = a_Position.w / 256.;

    float aoY = aoID * 16.;
    float aoX = fract(aoY);
    aoY = floor(aoY) / 16.;

    v_AmbientOcclusionCoord = vec2(aoX, aoY);
    v_AmbientOcclusionCoord += a_AmbientOcclusionCoordinate.xy * (15. / 256.);
    v_AmbientOcclusionCoord += .5 / 256.;

    ProcessSolidCommonVS(worldPosition, worldNormal);
}