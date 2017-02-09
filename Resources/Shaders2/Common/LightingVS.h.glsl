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

#pragma parameter c_IsRenderingMap

#pragma link <Shaders2/Common/LightingVS.glsl>

#if c_IsRenderingMap
    /**
     * @param worldPosition
     * @param fixedVertexCoord
     * @param normal The vertex normal.
     * @param dotNL The dot product of the vertex normal and the sun light vector
     */
    void PrepareLightingForMap(vec3 worldPosition, vec3 fixedVertexCoord, vec3 normal, float dotNL, mat4 viewMatrix, vec3 cameraPosition);
#else // c_IsRenderingMap
    void PrepareLighting(vec3 worldPosition, vec3 normal, mat4 viewMatrix, vec3 cameraPosition);
#endif