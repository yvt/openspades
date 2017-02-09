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

#pragma include <Shaders2/Common/FogFS.h.glsl>

varying vec3 v_fogExtinction;
varying vec3 v_fogInscattering;

vec3 ApplyFog(vec3 radiance) {
    return ApplyFogWithoutInscattering(radiance) + v_fogInscattering;
}

vec3 ApplyFogWithoutInscattering(vec3 radiance) {
    return radiance * v_fogExtinction;
}
