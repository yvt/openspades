/*
 Copyright (c) 2013 yvt

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



uniform vec3 fogColor;
varying float hemisphereLighting;

vec3 Radiosity_Map(float detailAmbientOcclusion, float ssao) {
	return mix(fogColor, vec3(1.), 0.5) *
	(0.5 * detailAmbientOcclusion * hemisphereLighting * ssao);
}

vec3 BlurredReflection_Map(float detailAmbientOcclusion, vec3 direction, float ssao) {
    return fogColor * ((direction.z * -0.5 + 0.5) * detailAmbientOcclusion * ssao);
}
