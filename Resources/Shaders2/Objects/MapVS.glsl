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

#pragma include <Shaders2/Common/LightingVS.h.glsl>

uniform mat4 u_ProjectionViewMatrix;
uniform mat4 u_ViewMatrix;
uniform vec3 u_ChunkPosition;
uniform vec3 u_CameraPosition;

// [x, y, z]
attribute vec3 a_Position;

// [ax, ay]
attribute vec2 a_AmbientOcclusionCoord;

// [R, G, B, diffuse]
attribute vec4 a_Color;

// [nx, ny, nz]
attribute vec3 a_Normal;

// [sx, sy, sz]
attribute vec3 a_VoxelPosition;

varying vec2 v_AmbientOcclusionCoord;
varying vec4 v_Color;

void main() {
    vec3 worldPosition = u_ChunkPosition + a_Position.xyz;

    gl_Position = u_ProjectionViewMatrix * vec4(worldPosition, 1.0);

    v_Color = a_Color.xyz * a_Color.xyz; // Linearize

    v_AmbientOcclusionCoord = (a_AmbientOcclusionCoord + .5) * (1. / 256.);

    vec3 fixedWorldPosition = u_ChunkPosition + a_VoxelPosition * 0.5 + a_Normal * 0.1;
    float dotNL = a_Color.w;

    PrepareLightingForMap(worldPosition, fixedWorldPosition, a_Normal,
        dotNL, u_ViewMatrix, mat3(u_ViewMatrix), u_CameraPosition);
}

