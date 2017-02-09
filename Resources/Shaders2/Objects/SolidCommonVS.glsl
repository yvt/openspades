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
uniform vec3 u_CameraPosition;

void ProcessSolidCommonVS(vec3 worldPosition, vec3 worldNormal) {
    gl_Position = u_ProjectionViewMatrix * vec4(worldPosition, 1.0);

    PrepareLighting(worldPosition, worldNormal, u_ViewMatrix, u_CameraPosition);

    // TODO: velocity buffer
}

