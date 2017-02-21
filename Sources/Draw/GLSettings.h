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
#pragma once

#include <Core/SettingSet.h>

namespace spades {
	namespace draw {
		class GLSettings : public SettingSet {
		public:
			GLSettings();

			TypedItemHandle<bool> r_blitFramebuffer     { *this, "r_blitFramebuffer" };
			TypedItemHandle<bool> r_bloom               { *this, "r_bloom" };
			TypedItemHandle<bool> r_cameraBlur          { *this, "r_cameraBlur", ItemFlags::Latch };
			TypedItemHandle<bool> r_colorCorrection     { *this, "r_colorCorrection" };
			TypedItemHandle<bool> r_debugTiming         { *this, "r_debugTiming" };
			TypedItemHandle<bool> r_debugTimingOutputScreen { *this, "r_debugTimingOutputScreen" };
			TypedItemHandle<bool> r_debugTimingOutputLog { *this, "r_debugTimingOutputLog" };
			TypedItemHandle<bool> r_debugTimingAverage  { *this, "r_debugTimingAverage" };
			TypedItemHandle<bool> r_debugTimingGPUTime  { *this, "r_debugTimingGPUTime" };
			TypedItemHandle<float> r_debugTimingOutputBarScale { *this, "r_debugTimingOutputBarScale" };
			TypedItemHandle<bool> r_debugTimingFlush    { *this, "r_debugTimingFlush" };
			TypedItemHandle<bool> r_debugTimingFillGap  { *this, "r_debugTimingFillGap" };
			TypedItemHandle<int> r_depthOfField         { *this, "r_depthOfField" };
			TypedItemHandle<float> r_depthOfFieldMaxCoc { *this, "r_depthOfFieldMaxCoc" };
			TypedItemHandle<bool> r_depthPrepass        { *this, "r_depthPrepass" };
			TypedItemHandle<bool> r_dlights             { *this, "r_dlights" };
			TypedItemHandle<float> r_exposureValue      { *this, "r_exposureValue" };
			TypedItemHandle<bool> r_fogShadow           { *this, "r_fogShadow", ItemFlags::Latch };
			TypedItemHandle<bool> r_fxaa                { *this, "r_fxaa" };
			TypedItemHandle<bool> r_hdr                 { *this, "r_hdr", ItemFlags::Latch };
			TypedItemHandle<float> r_hdrAutoExposureMin { *this, "r_hdrAutoExposureMin" };
			TypedItemHandle<float> r_hdrAutoExposureMax { *this, "r_hdrAutoExposureMax" };
			TypedItemHandle<float> r_hdrAutoExposureSpeed{ *this, "r_hdrAutoExposureSpeed" };
			TypedItemHandle<float> r_hdrGamma           { *this, "r_hdrGamma" };
			TypedItemHandle<bool> r_highPrec            { *this, "r_highPrec", ItemFlags::Latch };
			TypedItemHandle<bool> r_lens                { *this, "r_lens" };
			TypedItemHandle<bool> r_lensFlare           { *this, "r_lensFlare" };
			TypedItemHandle<bool> r_lensFlareDynamic    { *this, "r_lensFlareDynamic" };
			TypedItemHandle<bool> r_mapSoftShadow       { *this, "r_mapSoftShadow", ItemFlags::Latch };
			TypedItemHandle<float> r_maxAnisotropy      { *this, "r_maxAnisotropy", ItemFlags::Latch };
			TypedItemHandle<bool> r_modelShadows        { *this, "r_modelShadows", ItemFlags::Latch };
			TypedItemHandle<int> r_multisamples         { *this, "r_multisamples", ItemFlags::Latch };
			TypedItemHandle<bool> r_occlusionQuery      { *this, "r_occlusionQuery" };
			TypedItemHandle<bool> r_optimizedVoxelModel { *this, "r_optimizedVoxelModel", ItemFlags::Latch };
			TypedItemHandle<bool> r_physicalLighting    { *this, "r_physicalLighting", ItemFlags::Latch };
			TypedItemHandle<int> r_radiosity            { *this, "r_radiosity", ItemFlags::Latch };
			TypedItemHandle<float> r_saturation         { *this, "r_saturation" };
			TypedItemHandle<int> r_shadowMapSize        { *this, "r_shadowMapSize", ItemFlags::Latch };
			TypedItemHandle<bool> r_softParticles       { *this, "r_softParticles" };
			TypedItemHandle<bool> r_sparseShadowMaps    { *this, "r_sparseShadowMaps", ItemFlags::Latch };
			TypedItemHandle<bool> r_srgb                { *this, "r_srgb", ItemFlags::Latch };
			TypedItemHandle<bool> r_srgb2D              { *this, "r_srgb2D", ItemFlags::Latch };
			TypedItemHandle<int> r_ssao                 { *this, "r_ssao", ItemFlags::Latch };
			TypedItemHandle<int> r_water                { *this, "r_water", ItemFlags::Latch };
		};
	}
}
