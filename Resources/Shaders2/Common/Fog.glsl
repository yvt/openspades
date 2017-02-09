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

#pragma include <Shaders2/Common/Fog.h.glsl>

uniform vec3 u_fogColor;

FogEvaluationResult ComputeFogDensity(float poweredLength) {
    return ComputeFogDensityWithNormalizedDistance(poweredLength * pow(1.0 / 128.0, 2.0));
}

FogEvaluationResult ComputeFogDensityWithNormalizedDistance(float poweredLengthNormalized) {
    float distance = min(poweredLength, 1.);
    float weakenedDensity = 1. - distance;
    weakenedDensity *= weakenedDensity;

    FogEvaluationResult result;
    result.extinction = mix(vec4(distance), vec4(1. - weakenedDensity), vec3(0., 0.3, 1.0));
    result.inscattering = (1.0 - result.extinction) * u_fogColor;
    return result;
}