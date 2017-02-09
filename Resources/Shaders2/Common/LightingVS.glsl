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

#pragma parameter c_LightingPass
#pragma parameter c_RadiosityMode
#pragma parameter c_ShowModelShadows
#pragma parameter c_IsRenderingMap
#pragma parameter c_UsePhysicallyBasedLighting

#pragma include <Shaders2/Common/LightingVS.h.glsl>

#pragma include <Shaders2/Common/Lighting.h.glsl>
#pragma include <Shaders2/Common/FogVS.h.glsl>

#if c_LightingPass == LightingPassStatic

    /*
     * Static light pass - sun light and environmental lighting
     */

    /*
     * Dynamic shadows casted by models
     */
#   if c_ShowModelShadows

        uniform mat4 u_ShadowMapMatrix;
        varying vec4 v_ShadowMapCoord;

        void TransformShadowMatrix(out vec4 shadowMapCoord,
                                   in vec3 vertexCoord,
                                   in mat4 matrix) {
            vec4 c;
            c = matrix * vec4(vertexCoord, 1.);
            c.xyz = (c.xyz * 0.5) + c.w * 0.5;
            // bias
            c.z -= c.w * 0.0003;
            shadowMapCoord = c;
        }

        void PrepareForModelShadow(vec3 vertexCoord, vec3 normal) {
            TransformShadowMatrix(v_ShadowMapCoord,
                                  vertexCoord,
                                  u_ShadowMapMatrix);

        }

#   else // c_ShowModelShadows

        void PrepareForModelShadow(vec3 vertexCoord, vec3 normal) {}

#   endif // c_ShowModelShadows

    /*
     * Map shadows
     */
    varying vec3 v_MapShadowCoord;

    void PrepareForMapShadow(vec3 vertexCoord, vec3 normal) {
        v_MapShadowCoord = vertexCoord;
        v_MapShadowCoord.y -= v_MapShadowCoord.z;

        // texture value is normalized unsigned integer
        v_MapShadowCoord.z /= 255.;

        // texture coord is normalized
        // FIXME: variable texture size
        v_MapShadowCoord.xy /= 512.;
    }

    /*
     * Radiosity
     */
#   if c_RadiosityMode != RadiosityModeNone

        varying vec3 v_RadiosityTextureCoord;
        varying vec3 v_AmbientShadowTextureCoord;
        varying vec3 v_NormalVarying;

        void PrepareForMapRadiosity(vec3 vertexCoord, vec3 normal) {

            v_RadiosityTextureCoord = (vertexCoord + vec3(0., 0., 0.)) / vec3(512., 512., 64.);
            v_AmbientShadowTextureCoord = (vertexCoord + vec3(0.5, 0.5, 1.5)) / vec3(512., 512., 65.);

            v_NormalVarying = normal;
        }

#   else // c_RadiosityMode != RadiosityModeNone

        varying float v_HemisphereLighting;

        void PrepareForMapRadiosity(vec3 vertexCoord, vec3 normal) {
            v_HemisphereLighting = 1. - normal.z * .2;
        }

#   endif // c_RadiosityMode != RadiosityModeNone

#   if !cc_HasBumpMaps
        varying float v_DotNL;
#       if c_UsePhysicallyBasedLighting
            varying vec3 v_ViewPosition;
            varying vec3 v_ViewNormal;
            void ComputeExtraVaryings(vec3 worldPosition, vec3 normal, mat4 viewMatrix) {
                v_ViewPosition = (vec4(worldPosition, 1.0) * viewMatrix).xyz;
                v_ViewNormal = (vec4(normal, 0.0) * viewMatrix).xyz;
            }
#       else // c_UsePhysicallyBasedLighting
            void ComputeExtraVaryings(vec3 worldPosition, vec3 normal, mat4 viewMatrix) {}
#       endif // c_UsePhysicallyBasedLighting
#   else // !cc_HasBumpMaps
#       error bump maps aren't supported yet
#   endif // !cc_HasBumpMaps

#   if c_IsRenderingMap

        // map uses specialized shadow coordinate calculation to avoid glitch
        void PrepareLightingForMap(vec3 worldPosition, vec3 fixedVertexCoord, vec3 normal, float dotNL, mat4 viewMatrix, vec3 cameraPosition) {
            PrepareForMapShadow(fixedVertexCoord, normal);
            PrepareForModelShadow(worldPosition, normal);
            PrepareForMapRadiosity(worldPosition, normal);
#       if !cc_HasBumpMaps
            v_DotNL = dotNL;
#       endif
            ComputeExtraVaryings(worldPosition, normal, viewMatrix);
            PrepareFog(worldPosition, cameraPosition);
        }

#   else // c_IsRenderingMap


        void PrepareLighting(vec3 worldPosition, vec3 normal, mat4 viewMatrix, vec3 cameraPosition) {
            PrepareForMapShadow(worldPosition, normal);
            PrepareForModelShadow(worldPosition, normal);
            PrepareForMapRadiosity(worldPosition, normal);
#           if !cc_HasBumpMaps
                v_DotNL = dot(normal, vec3(0.0, -1.0, -1.0)); // TODO: check light direction
#           endif
            ComputeExtraVaryings(worldPosition, normal, viewMatrix);
            PrepareFog(worldPosition, cameraPosition);
        }

#   endif // c_IsRenderingMap

#elif c_LightingPass == LightingPassDynamic

    /*
     * Dynamic light pass - dynamic lights
     */

    uniform vec3 u_DynamicLightOrigin;
    uniform mat4 u_DynamicLightSpotMatrix;

    varying vec3 v_LightPos;
    varying vec3 v_LightNormal;
    varying vec3 v_LightTexCoord;

    void PrepareLighting(vec3 worldPosition, vec3 normal, mat4 viewMatrix, vec3 cameraPosition) {
        v_LightPos = u_DynamicLightOrigin - worldPosition;
        v_LightNormal = normal;

        // projection
        v_LightTexCoord = (u_DynamicLightSpotMatrix * vec4(worldPosition, 1.0)).xyw;

        PrepareFog(worldPosition, cameraPosition);
    }

    void PrepareLightingForMap(vec3 worldPosition, vec3 fixedVertexCoord, vec3 normal, float dotNL, mat4 viewMatrix, vec3 cameraPosition) {
        PrepareLighting(worldPosition, normal, viewMatrix);
    }

#elif c_LightingPass == LightingPassShadowMap || c_LightingPass == LightingPassPrepass

    /*
     * Prepass - lighting is disabled
     */

    void PrepareLighting(vec3 worldPosition, vec3 normal, mat4 viewMatrix, vec3 cameraPosition) {}
    void PrepareLightingForMap(vec3 worldPosition, vec3 fixedVertexCoord, vec3 normal, float dotNL, mat4 viewMatrix, vec3 cameraPosition) {}

#endif // c_LightingPass == LightingPassStatic

