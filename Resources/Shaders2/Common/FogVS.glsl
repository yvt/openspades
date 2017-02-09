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

#pragma include <Shaders2/Common/FogVS.h.glsl>

#pragma include <Shaders2/Common/Fog.h.glsl>

varying vec3 v_fogExtinction;
varying vec3 v_fogInscattering;

void PrepareFog(vec3 worldPosition, vec3 cameraPosition) {
    // No vertical fog in the original AoS
    float fogDistance = length(worldPosition.xy - cameraPosition.xy);

    FogEvaluationResult result = EvaluateFog(fogDistance);
    v_fogInscattering = result.inscattering;
    v_fogExtinction = result.extinction;
}
