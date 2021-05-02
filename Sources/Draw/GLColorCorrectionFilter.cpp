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

#include <vector>
#include <cmath>

#include "GLColorCorrectionFilter.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>
#include <Core/Settings.h>

namespace spades {
	namespace draw {
		GLColorCorrectionFilter::GLColorCorrectionFilter(GLRenderer &renderer)
		    : renderer(renderer), settings(renderer.GetSettings()) {
			lens = renderer.RegisterProgram("Shaders/PostFilters/ColorCorrection.program");
			gaussProgram = renderer.RegisterProgram("Shaders/PostFilters/Gauss1D.program");
		}
		GLColorBuffer GLColorCorrectionFilter::Filter(GLColorBuffer input, Vector3 tintVal,
		                                              float fogLuminance) {
			SPADES_MARK_FUNCTION();

			IGLDevice &dev = renderer.GetGLDevice();
			GLQuadRenderer qr(dev);

			GLColorBuffer output = input.GetManager()->CreateBufferHandle();

			float sharpeningFinalGainValue =
			  std::max(std::min(settings.r_sharpen.operator float(), 1.0f), 0.0f);
			GLColorBuffer blurredInput = input;

			if (sharpeningFinalGainValue > 0.0f) {
				// Apply a 1D gaussian blur on the horizontal direction.
				// (The vertical direction blur is done by the final program)
				static GLProgramAttribute blur_positionAttribute("positionAttribute");
				static GLProgramUniform blur_textureUniform("mainTexture");
				static GLProgramUniform blur_unitShift("unitShift");
				gaussProgram->Use();
				blur_positionAttribute(gaussProgram);
				blur_textureUniform(gaussProgram);
				blur_unitShift(gaussProgram);
				blur_textureUniform.SetValue(0);

				dev.ActiveTexture(0);
				qr.SetCoordAttributeIndex(blur_positionAttribute());
				dev.Enable(IGLDevice::Blend, false);

				blurredInput = renderer.GetFramebufferManager()->CreateBufferHandle();
				dev.BindTexture(IGLDevice::Texture2D, input.GetTexture());
				dev.BindFramebuffer(IGLDevice::Framebuffer, blurredInput.GetFramebuffer());
				blur_unitShift.SetValue(1.0f / (float)input.GetWidth(), 0.f);
				qr.Draw();
			}

			float sharpeningFloor = 0.0f;

			// If temporal AA is enabled, enable the sharpening effect regardless of
			// the current fog color to offset the blurring caused by the temporal AA.
			if (settings.r_temporalAA) {
				sharpeningFloor = 1.5f;
			}

			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensTexture("mainTexture");
			static GLProgramUniform blurredTexture("blurredTexture");

			static GLProgramUniform saturation("saturation");
			static GLProgramUniform enhancement("enhancement");
			static GLProgramUniform tint("tint");
			static GLProgramUniform sharpening("sharpening");
			static GLProgramUniform sharpeningFinalGain("sharpeningFinalGain");
			static GLProgramUniform blurPixelShift("blurPixelShift");

			saturation(lens);
			enhancement(lens);
			tint(lens);
			sharpening(lens);
			sharpeningFinalGain(lens);
			blurPixelShift(lens);

			dev.Enable(IGLDevice::Blend, false);

			lensPosition(lens);
			lensTexture(lens);
			blurredTexture(lens);

			lens->Use();

			tint.SetValue(tintVal.x, tintVal.y, tintVal.z);

			const client::SceneDefinition &def = renderer.GetSceneDef();

			if (settings.r_hdr) {
				// when HDR is enabled ACES tone mapping is applied first, so
				// lower enhancement value is required
				if (settings.r_bloom) {
					saturation.SetValue(0.8f * def.saturation * settings.r_saturation);
					enhancement.SetValue(0.1f);
				} else {
					saturation.SetValue(0.9f * def.saturation * settings.r_saturation);
					enhancement.SetValue(0.0f);
				}
			} else {
				if (settings.r_bloom) {
					// make image sharper
					saturation.SetValue(.85f * def.saturation * settings.r_saturation);
					enhancement.SetValue(0.7f);
				} else {
					saturation.SetValue(1.f * def.saturation * settings.r_saturation);
					enhancement.SetValue(0.3f);
				}
			}

			lensTexture.SetValue(0);
			blurredTexture.SetValue(1);

			// Calculate the sharpening factor
			//
			// One reason to do this is for aesthetic reasons. Another reason is to offset
			// OpenSpades' denser fog compared to the vanilla client. Technically, the fog density
			// function is mostly identical between these two clients. However, OpenSpades applies
			// the fog color in the linear color space, which is physically accurate but has an
			// unexpected consequence of somewhat strengthening the effect.
			//
			// (`r_volumetricFog` completely changes the density function, which we leave out from
			// this discussion.)
			//
			// Given an object color o (only one color channel is discussed here), fog color f, and
			// fog density d, the output color c_voxlap and c_os for the vanilla client and
			// OpenSpades, respectively, is calculated by:
			//
			//     c_voxlap = o^(1/2)(1-d) + f^(1/2)d
			//         c_os = (o(1-d) + fd)^(1/2)
			//
			// Here the sRGB transfer function is approximated by an exact gamma = 2 power law.
			// o and f are in the linear color space, whereas c_voxlap and c_os are in the sRGB
			// color space (because that's how `ColorCorrection.fs` is implemented).
			//
			// The contrast reduction by the fog can be calculated by differentiating each of them
			// by o:
			//
			//     c_voxlap' = (1-d) / sqrt(o) / 2
			//         c_os' = (1-d) / sqrt(o(1-d) + fd) / 2
			//
			// Now we find out the amount of color contrast we must recover by dividing c_voxlap' by
			// c_os'. Since it's objects around the fog end distance that concern the users, let
			// d = 1:
			//
			//   c_voxlap' / c_os' = sqrt(o(1-d) + fd) / sqrt(o)
			//                     = sqrt(f) / sqrt(o)
			//
			// (Turns out, the result so far does not change whichever color space c_voxlap and c_os
			// are represented in.)
			//
			// This is a function over an object color o and fog color f. Let us calculate the
			// average of this function assuming a uniform distribution of o over the interval
			// [o_min, o_max]:
			//
			//   ∫[c_voxlap' / c_os', {o, o_min, o_max}]
			//       = 2sqrt(f)(sqrt(o_max) - sqrt(o_min)) / (o_max - o_min)
			//
			// Since the pixels aren't usually fully lit nor completely dark, let us arbitrarily
			// assume o_min = 0.001 and o_max = 0.5 (I think this is reasonable for a deuce hiding
			// in a shady corridor) (and let it be `r_offset`):
			//
			//   r_offset
			//    = 2sqrt(f)(sqrt(o_max) - sqrt(o_min)) / (o_max - o_min)
			//    ≈ 2.70 sqrt(f)
			//
			// So if this value is higher than 1, we need enhance the rendered image. Otherwise,
			// we will maintain the status quo for now. (In most servers I have encountered, the fog
			// color was a bright color, so this status quo won't be a problem, I think. No one has
			// complained about it so far.)
			sharpening.SetValue(std::max(std::sqrt(fogLuminance) * 2.7f, sharpeningFloor));
			sharpeningFinalGain.SetValue(sharpeningFinalGainValue);
			blurPixelShift.SetValue(1.0f / (float)input.GetHeight());

			// composite to the final image

			qr.SetCoordAttributeIndex(lensPosition());
			dev.ActiveTexture(1);
			dev.BindTexture(IGLDevice::Texture2D, blurredInput.GetTexture());
			dev.ActiveTexture(0);
			dev.BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev.BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev.Viewport(0, 0, output.GetWidth(), output.GetHeight());
			qr.Draw();
			dev.BindTexture(IGLDevice::Texture2D, 0);

			return output;
		}
	} // namespace draw
} // namespace spades
