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

#pragma link <Shaders2/Common/ShadingModels.glsl>

struct PrincipledBRDFParameter {
    // There are fewer parameters than the original paper because
    // the variation of materials in OpenSpades is quite limited
    vec3 diffuseColor;
    float roughness;
};

float BeckmannDistribution(float m, float dotHalf);
float GGXDistribution(float m, float dotHalf);
float CockTorrance(vec3 eyeVec, vec3 lightVec, vec3 normal);
float OrenNayar(float sigma, float dotLight, float dotEye);
