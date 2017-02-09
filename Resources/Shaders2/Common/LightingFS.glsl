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
#pragma parameter c_UseVarianceShadowMaps
#pragma parameter c_UseSSAO
#pragma parameter c_UsePhysicallyBasedLighting
#pragma parameter c_IsRenderingMap
#pragma parameter c_UsePostprocessFog"

#pragma include <Shaders2/Common/LightingFS.h.glsl>

#pragma include <Shaders2/Common/ShadingModels.h.glsl>
#pragma include <Shaders2/Common/Lighting.h.glsl>
#pragma include <Shaders2/Common/FogFS.h.glsl>

#if c_LightingPass == LightingPassStatic

    /*
     * Static light pass - sun light and environmental lighting
     */

    /*
     * Dynamic shadows casted by models
     */
#   if c_ShowModelShadows

        uniform sampler2DShadow u_ShadowMapTexture1;
        uniform sampler2D u_ShadowMapTexture2;

        uniform float u_PagetableSize;
        uniform float u_PagetableSizeInv;
        uniform float u_MinLod;
        uniform float u_ShadowMapSizeInv;

        varying vec4 v_ShadowMapCoord;

        float EvaluteModelShadow() {
            vec4 scoord = v_ShadowMapCoord.xyzw;

            vec2 pagetableFract = fract(scoord.xy * u_PagetableSize);
            vec2 pagetableInt = floor(scoord.xy * u_PagetableSize);
            vec4 mapData = texture2D(u_ShadowMapTexture2, pagetableInt * u_PagetableSizeInv);

            // decode map size
            if(mapData.w > .99) return 1.; // no shadow map
            mapData *= 255.01;

            vec2 physCoord = mapData.xy;
            physCoord = floor(physCoord);

            physCoord.x += floor(mod(mapData.z, 16.)) * 256.;
            physCoord.y += floor(mapData.z / 16.) * 256.;

            //float lod = mapData.w * u_MinLod;
            float lod = exp2(floor(mapData.w));
            physCoord.xy += lod * pagetableFract;

            physCoord.xy *= u_ShadowMapSizeInv;

            vec3 physCoord2 = vec3(physCoord, scoord.z);

            float v = shadow2D(u_ShadowMapTexture1, physCoord2).x;
            return v;
        }

#   else // c_ShowModelShadows

        float EvaluteModelShadow() { return 1.0; }

#   endif // c_ShowModelShadows

    /*
     * Map shadows
     */
    uniform sampler2D mapShadowTexture;
    varying vec3 mapShadowCoord;

#   if c_UseVarianceShadowMaps

        float EvaluateMapShadow() {
            const vec2 mapSize = vec2(512.); // TODO: variable?
            vec2 mapSizeInv = 1. / mapSize;

            vec2 shadowMapPixCoord = mapShadowCoord.xy * mapSize;
            vec2 shadowMapPixInt = floor(shadowMapPixCoord);
            vec2 shadowMapPixFract = fract(shadowMapPixCoord);
            vec2 shadowMapBlend = shadowMapPixFract - 0.5;
            vec2 shadowMapNeighborSign = sign(shadowMapBlend);
            shadowMapBlend = abs(shadowMapBlend);

            vec4 shadowMapSampleCoords = shadowMapPixInt.xyxy;
            shadowMapSampleCoords.zw += shadowMapNeighborSign;
            shadowMapSampleCoords *= mapSizeInv.xyxy;

            vec4 samples = vec4(
                texture2D(mapShadowTexture, shadowMapSampleCoords.xy).w,
                texture2D(mapShadowTexture, shadowMapSampleCoords.zy).w,
                texture2D(mapShadowTexture, shadowMapSampleCoords.xw).w,
                texture2D(mapShadowTexture, shadowMapSampleCoords.zw).w
            );

            vec2 average1 = mix(samples.xz, samples.yw, shadowMapBlend.x);
            float average = mix(average1.x, average1.y, shadowMapBlend.y);

            if(average > mapShadowCoord.z)
                return 1.;

            vec4 samples2 = samples * samples;
            vec2 average2 = mix(samples2.xz, samples2.yw, shadowMapBlend.x);
            float averageP = mix(average2.x, average2.y, shadowMapBlend.y);

            float variance = averageP - average * average;
            variance = max(variance, 0.000000001);

            float val = mapShadowCoord.z - average;
            val *= val;
            val = variance / (variance + val);

            return val;
        }

#   else // c_UseVarianceShadowMaps

        float EvaluateMapShadow() {
            float val = texture2D(mapShadowTexture, mapShadowCoord.xy).w;
            if(val < mapShadowCoord.z - 0.0001)
                return 0.;
            else
                return 1.;
        }

#   endif


    /*
     * Radiosity
     */
#   if c_RadiosityMode != RadiosityModeNone

        uniform sampler3D ambientShadowTexture;
        uniform sampler3D radiosityTextureFlat;
        uniform sampler3D radiosityTextureX;
        uniform sampler3D radiosityTextureY;
        uniform sampler3D radiosityTextureZ;
        varying vec3 radiosityTextureCoord;
        varying vec3 ambientShadowTextureCoord;
        varying vec3 normalVarying;
        uniform vec3 ambientColor;

#       if c_RadiosityMode == RadiosityModeLowPrecision
            vec3 DecodeRadiosityValue(vec3 val){
                // reverse bias
                val *= 1023. / 1022.;
                val = (val * 2.) - 1.;
                val *= val * sign(val);
                return val;
            }
#       else // c_RadiosityMode == RadiosityModeLowPrecision
            vec3 DecodeRadiosityValue(vec3 val){
                // reverse bias
                val *= 1023. / 1022.;
                val = (val * 2.) - 1.;
                return val;
            }
#       endif

        vec3 EvaluateRadiosity(float detailAmbientOcclusion, float ssao) {
            vec3 col = DecodeRadiosityValue
            (texture3D(radiosityTextureFlat,
                       radiosityTextureCoord).xyz);
            vec3 normal = normalize(normalVarying);
            col += normal.x * DecodeRadiosityValue
            (texture3D(radiosityTextureX,
                       radiosityTextureCoord).xyz);
            col += normal.y * DecodeRadiosityValue
            (texture3D(radiosityTextureY,
                       radiosityTextureCoord).xyz);
            col += normal.z * DecodeRadiosityValue
            (texture3D(radiosityTextureZ,
                       radiosityTextureCoord).xyz);
            col = max(col, 0.);
            col *= 1.5 * ssao;

            detailAmbientOcclusion *= ssao;

            // ambient occlusion
            float amb = texture3D(ambientShadowTexture, ambientShadowTextureCoord).x;
            amb = max(amb, 0.); // for some reason, mainTexture value becomes negative

            // mix ambient occlusion values generated by two different methods somehow
            amb = mix(sqrt(amb * detailAmbientOcclusion), min(amb, detailAmbientOcclusion), 0.5);

            amb *= .8 - normalVarying.z * .2;
            col += amb * ambientColor;

            return col;
        }

        vec3 EvaluateSoftReflections(float detailAmbientOcclusion, vec3 direction, float ssao) {
            vec3 col = DecodeRadiosityValue
            (texture3D(radiosityTextureFlat,
                       radiosityTextureCoord).xyz);
            vec3 normal = normalize(normalVarying);
            col += normal.x * DecodeRadiosityValue
            (texture3D(radiosityTextureX,
                       radiosityTextureCoord).xyz);
            col += normal.y * DecodeRadiosityValue
            (texture3D(radiosityTextureY,
                       radiosityTextureCoord).xyz);
            col += normal.z * DecodeRadiosityValue
            (texture3D(radiosityTextureZ,
                       radiosityTextureCoord).xyz);
            col = max(col, 0.);
            col *= 1.5 * ssao;

            detailAmbientOcclusion *= ssao;

            // ambient occlusion
            float amb = texture3D(ambientShadowTexture, ambientShadowTextureCoord).x;
            amb = max(amb, 0.); // for some reason, mainTexture value becomes negative
            amb *= amb; // darken

            // mix ambient occlusion values generated by two different methods somehow
            amb = mix(sqrt(amb * detailAmbientOcclusion), min(amb, detailAmbientOcclusion), 0.5);

            amb *= .8 - normalVarying.z * .2;

            return col + ambientColor * amb;
        }

#   else // c_RadiosityMode != RadiosityModeNone

        uniform vec3 u_AmbientColor;
        varying float v_HemisphereLighting;

        vec3 EvaluateRadiosity(float detailAmbientOcclusion, float ssao) {
            return mix(u_AmbientColor, vec3(1.), 0.5) *
            (0.5 * detailAmbientOcclusion * v_HemisphereLighting * ssao);
        }

        vec3 EvaluateSoftReflections(float detailAmbientOcclusion, vec3 direction, float ssao) {
            return u_AmbientColor * ((direction.z * -0.5 + 0.5) * detailAmbientOcclusion * ssao);
        }

#   endif // c_RadiosityMode != RadiosityModeNone

    float VisibilityOfSunLight() {
        return VisibilityOfSunLight_Map() *
        VisibilityOfSunLight_Model();
    }

    vec3 EvaluateAmbientLight(float detailAmbientOcclusion) {
#       if c_UseSSAO
            float ssao = texture2D(ssaoTexture, gl_FragCoord.xy * ssaoTextureUVScale).x;
#       else // c_UseSSAO
            float ssao = 1.0;
#       endif // c_UseSSAO
        return Radiosity_Map(detailAmbientOcclusion, ssao);
    }

    vec3 EvaluateDirectionalAmbientLight(float detailAmbientOcclusion, vec3 direction) {
#       if c_UseSSAO
            float ssao = texture2D(ssaoTexture, gl_FragCoord.xy * ssaoTextureUVScale).x;
#       else // c_UseSSAO
            float ssao = 1.0;
#       endif // c_UseSSAO
        return BlurredReflection_Map(detailAmbientOcclusion, direction, ssao);
    }

    uniform vec3 u_viewSpaceLight;

#   if !cc_HasBumpMaps
        varying float v_DotNL;
#       if c_UsePhysicallyBasedLighting
            varying vec3 v_ViewPosition;
            varying vec3 v_ViewNormal;
#       endif // c_UsePhysicallyBasedLighting
#   else // !cc_HasBumpMaps
#       error bump maps aren't supported yet
#   endif // !cc_HasBumpMaps

    vec3 EvaluateLighting(PrincipledBRDFParameter brdf, float detailAmbientOcclusion) {
        float dotNL = v_DotNL;
        vec3 diffuseShading = EvaluateAmbientLight(detailAmbientOcclusion);
        float shadowing = VisibilityOfSunLight() * 0.6;
        vec3 radiance;

#       if c_UsePhysicallyBasedLighting
            vec3 eye = normalize(v_ViewPosition);
            float dotNE = dot(v_ViewNormal, eye);

            // Fresnel term (be careful; v_ViewNormal is a surface normal, not a microfacet one)
            float fresnel2 = 1. + dotNE;
            float fresnel = 0.03 + 0.1 * fresnel2 * fresnel2;

            // Specular shading (blurred reflections, assuming roughness is high)
            vec3 reflectionDir = reflect(); // TODO
            vec3 specularShading = EvaluateDirectionalAmbientLight(detailAmbientOcclusion, reflectionDir);

            // Diffuse/specular shading for sunlight
            if (shadowing > 0.0 && dotNL > 0.0) {
                // Diffuse shading
                float sunDiffuseShading = OrenNayar(brdf.roughness, dotNL, -dotNE);
                diffuseShading += sunDiffuseShading * shadowing;

                // Specular shading
                float sunSpecularShading = CockTorrance(-eye, u_viewSpaceLight, v_ViewNormal);
                specularShading += sunSpecularShading * shadowing;
            }

            radiance = mix(diffuseShading * brdf.diffuseColor, specularShading, fresnel);

#       else // c_UsePhysicallyBasedLighting
            diffuseShading += max(0.0, dotNL * shadowing); // FIXME: this `max` should be moved to VS
            radiance = diffuseShading * brdf.diffuseColor;
#       endif // c_UsePhysicallyBasedLighting

#       if c_UsePostprocessFog
            return ApplyFogWithoutInscattering(radiance);
#       else // c_UsePostprocessFog
            return ApplyFog(radiance);
#       endif // c_UsePostprocessFog
    }

#elif c_LightingPass == LightingPassDynamic

    /*
     * Dynamic light pass - dynamic lights
     */

    uniform vec3 u_DynamicLightColor;
    uniform float u_DynamicLightRadius;
    uniform float u_DynamicLightRadiusInversed;
    uniform sampler2D u_DynamicLightProjectionTexture;

    varying vec3 v_LightPos;
    varying vec3 v_LightNormal;
    varying vec3 v_LightTexCoord;

    vec3 EvaluateDynamicLightNoBump() {
        if (v_LightTexCoord.z < 0.) {
            discard;
        }

        // TODO: optimize
        // TODO: support physically based shading

        // Diffuse lighting
        float intensity = dot(normalize(v_LightPos), normalize(v_LightNormal));
        if (intensity < 0.) {
            discard;
        }

        // Attenuation
        float distance = length(v_LightPos);
        if(distance >= dynamicLightRadius){
            discard;
        }
        distance = max(1.0 - distance * dynamicLightRadiusInversed, 0.0);
        float attenuation = distance * distance;

        // Apply attenuation
        intensity *= attenuation;

        // Projection
        // if(v_LightTexCoord.w < 0.) discard; -- done earlier
        vec3 texValue = texture2DProj(u_DynamicLightProjectionTexture, v_LightTexCoord).xyz;

        return u_DynamicLightColor * intensity * texValue;
    }

    vec3 EvaluateLighting(PrincipledBRDFParameter brdf, float detailAmbientOcclusion) {
        vec3 radiance = EvaluateDynamicLightNoBump() * brdf.diffuseColor;
        return ApplyFogWithoutInscattering(radiance);
    }

#elif c_LightingPass == LightingPassShadowMap || c_LightingPass == LightingPassPrepass

    /*
     * Prepass - lighting is disabled
     */

    vec3 EvaluateLighting(PrincipledBRDFParameter brdf, float detailAmbientOcclusion) {
        return vec3(0.0);
    }

#endif // c_LightingPass == LightingPassStatic
