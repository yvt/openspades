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

#include <vector>

#include "GLFXAAFilter.h"
#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "GLTemporalAAFilter.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {
		GLTemporalAAFilter::GLTemporalAAFilter(GLRenderer &renderer) : renderer(renderer) {
			prevMatrix = Matrix4::Identity();
			prevViewOrigin = Vector3(0.0f, 0.0f, 0.0f);
			program = renderer.RegisterProgram("Shaders/PostFilters/TemporalAA.program");

			// Preload
			GLFXAAFilter{renderer};
		}

		GLTemporalAAFilter::~GLTemporalAAFilter() { DeleteHistoryBuffer(); }

		void GLTemporalAAFilter::DeleteHistoryBuffer() {
			if (!historyBuffer.valid) {
				return;
			}

			IGLDevice &dev = renderer.GetGLDevice();
			dev.DeleteFramebuffer(historyBuffer.framebuffer);
			dev.DeleteTexture(historyBuffer.texture);

			historyBuffer.valid = false;
		}

		GLColorBuffer GLTemporalAAFilter::Filter(GLColorBuffer input, bool useFxaa) {
			SPADES_MARK_FUNCTION();

			IGLDevice &dev = renderer.GetGLDevice();
			GLQuadRenderer qr(dev);

			// Calculate the current view-projection matrix.
			const client::SceneDefinition &def = renderer.GetSceneDef();
			Matrix4 newMatrix = Matrix4::Identity();
			Vector3 axes[] = {def.viewAxis[0], def.viewAxis[1], def.viewAxis[2]};
			newMatrix.m[0] = axes[0].x;
			newMatrix.m[1] = axes[1].x;
			newMatrix.m[2] = -axes[2].x;
			newMatrix.m[4] = axes[0].y;
			newMatrix.m[5] = axes[1].y;
			newMatrix.m[6] = -axes[2].y;
			newMatrix.m[8] = axes[0].z;
			newMatrix.m[9] = axes[1].z;
			newMatrix.m[10] = -axes[2].z;

			Matrix4 projectionMatrix;
			{
				// From `GLRenderer::BuildProjectionMatrix`
				float near = def.zNear;
				float far = def.zFar;
				float t = near * std::tan(def.fovY * .5f);
				float r = near * std::tan(def.fovX * .5f);
				float a = r * 2.f, b = t * 2.f, c = far - near;
				Matrix4 &mat = projectionMatrix;
				mat.m[0] = near * 2.f / a;
				mat.m[1] = 0.f;
				mat.m[2] = 0.f;
				mat.m[3] = 0.f;
				mat.m[4] = 0.f;
				mat.m[5] = near * 2.f / b;
				mat.m[6] = 0.f;
				mat.m[7] = 0.f;
				mat.m[8] = 0.f;
				mat.m[9] = 0.f;
				mat.m[10] = -(far + near) / c;
				mat.m[11] = -1.f;
				mat.m[12] = 0.f;
				mat.m[13] = 0.f;
				mat.m[14] = -(far * near * 2.f) / c;
				mat.m[15] = 0.f;
			}

			newMatrix = projectionMatrix * newMatrix;

			// In `y = newMatrix * x`, the coordinate space `y` belongs to must
			// cover the clip region by range `[0, 1]` (like texture coordinates)
			// instead of `[-1, 1]` (like OpenGL clip coordinates)
			newMatrix = Matrix4::Translate(1.0f, 1.0f, 1.0f) * newMatrix;
			newMatrix = Matrix4::Scale(0.5f, 0.5f, 0.5f) * newMatrix;

			// Camera translation must be incorporated into the calculation
			// separately to avoid numerical errors. (You'd be suprised to see
			// how visible the visual artifacts can be.)
			Matrix4 translationMatrix = Matrix4::Translate(def.viewOrigin - prevViewOrigin);

			// Compute the reprojection matrix
			Matrix4 inverseNewMatrix = newMatrix.Inversed();
			Matrix4 diffMatrix = prevMatrix * translationMatrix * inverseNewMatrix;
			prevMatrix = newMatrix;
			prevViewOrigin = def.viewOrigin;

			if (!historyBuffer.valid || historyBuffer.width != input.GetWidth() ||
			    historyBuffer.height != input.GetHeight()) {
				DeleteHistoryBuffer();

				historyBuffer.width = input.GetWidth();
				historyBuffer.height = input.GetHeight();

				auto internalFormat = renderer.GetFramebufferManager()->GetMainInternalFormat();

				historyBuffer.framebuffer = dev.GenFramebuffer();
				dev.BindFramebuffer(IGLDevice::Framebuffer, historyBuffer.framebuffer);

				historyBuffer.texture = dev.GenTexture();
				dev.BindTexture(IGLDevice::Texture2D, historyBuffer.texture);

				historyBuffer.valid = true;

				SPLog("Creating a history buffer");
				dev.TexImage2D(IGLDevice::Texture2D, 0, internalFormat, historyBuffer.width,
				               historyBuffer.height, 0, IGLDevice::RGBA, IGLDevice::UnsignedByte,
				               NULL);
				SPLog("History buffer allocated");
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                 IGLDevice::Linear);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                 IGLDevice::ClampToEdge);
				dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                 IGLDevice::ClampToEdge);

				dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
				                         IGLDevice::Texture2D, historyBuffer.texture, 0);

				IGLDevice::Enum status = dev.CheckFramebufferStatus(IGLDevice::Framebuffer);
				if (status != IGLDevice::FramebufferComplete) {
					SPRaise("Failed to create a history buffer.");
				}
				SPLog("Created a history framebuffer");

				// Initialize the history buffer with the latest input
				dev.BindFramebuffer(IGLDevice::DrawFramebuffer, historyBuffer.framebuffer);
				dev.BindFramebuffer(IGLDevice::ReadFramebuffer, input.GetFramebuffer());
				dev.BlitFramebuffer(0, 0, input.GetWidth(), input.GetHeight(), 0, 0,
				                    input.GetWidth(), input.GetHeight(), IGLDevice::ColorBufferBit,
				                    IGLDevice::Nearest);
				dev.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
				dev.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);

				// Reset the blending factor
				dev.BindFramebuffer(IGLDevice::Framebuffer, historyBuffer.framebuffer);
				dev.ColorMask(false, false, false, true);
				dev.ClearColor(0.0f, 0.0f, 0.0f, 0.5f);
				dev.Clear(IGLDevice::ColorBufferBit);
				dev.ColorMask(true, true, true, true);

				if (useFxaa) {
					return GLFXAAFilter{renderer}.Filter(input);
				}

				return input;
			}

			GLColorBuffer output = input.GetManager()->CreateBufferHandle();

			GLColorBuffer processedInput = input;
			/* // this didn't work well:
			GLColorBuffer processedInput =
			    useFxaa ? [&] {
			    GLProfiler::Context p(renderer.GetGLProfiler(), "FXAA");
			    return GLFXAAFilter{renderer}.Filter(input);
			}() : input; */

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramUniform inputTexture("inputTexture");
			static GLProgramUniform depthTexture("depthTexture");
			static GLProgramUniform previousTexture("previousTexture");
			static GLProgramUniform processedInputTexture("processedInputTexture");
			static GLProgramUniform reprojectionMatrix("reprojectionMatrix");
			static GLProgramUniform inverseVP("inverseVP");
			static GLProgramUniform viewProjectionMatrixInv("viewProjectionMatrixInv");
			static GLProgramUniform fogDistance("fogDistance");

			dev.Enable(IGLDevice::Blend, false);

			positionAttribute(program);
			inputTexture(program);
			depthTexture(program);
			previousTexture(program);
			processedInputTexture(program);
			reprojectionMatrix(program);
			inverseVP(program);
			viewProjectionMatrixInv(program);
			fogDistance(program);

			program->Use();

			inputTexture.SetValue(0);
			previousTexture.SetValue(1);
			processedInputTexture.SetValue(2);
			depthTexture.SetValue(3);
			reprojectionMatrix.SetValue(diffMatrix);
			inverseVP.SetValue(1.f / input.GetWidth(), 1.f / input.GetHeight());
			viewProjectionMatrixInv.SetValue(inverseNewMatrix);
			fogDistance.SetValue(128.f);

			// Perform temporal AA
			// TODO: pre/post tone mapping to prevent aliasing near overbright area
			qr.SetCoordAttributeIndex(positionAttribute());
			dev.ActiveTexture(0);
			dev.BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev.ActiveTexture(1);
			dev.BindTexture(IGLDevice::Texture2D, historyBuffer.texture);
			dev.ActiveTexture(2);
			dev.BindTexture(IGLDevice::Texture2D, processedInput.GetTexture());
			dev.ActiveTexture(3);
			dev.BindTexture(IGLDevice::Texture2D,
			                renderer.GetFramebufferManager()->GetDepthTexture());
			dev.ActiveTexture(0);
			dev.BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev.Viewport(0, 0, output.GetWidth(), output.GetHeight());
			qr.Draw();
			dev.BindTexture(IGLDevice::Texture2D, 0);

			// Copy the result to the history buffer
			dev.BindFramebuffer(IGLDevice::DrawFramebuffer, historyBuffer.framebuffer);
			dev.BindFramebuffer(IGLDevice::ReadFramebuffer, output.GetFramebuffer());
			dev.BlitFramebuffer(0, 0, input.GetWidth(), input.GetHeight(), 0, 0, input.GetWidth(),
			                    input.GetHeight(), IGLDevice::ColorBufferBit, IGLDevice::Nearest);
			dev.BindFramebuffer(IGLDevice::ReadFramebuffer, 0);
			dev.BindFramebuffer(IGLDevice::DrawFramebuffer, 0);
			return output;
		}

		Vector2 GLTemporalAAFilter::GetProjectionMatrixJitter() {
			// Obtained from Hyper3D (MIT licensed)
			static const std::vector<float> jitterTable = {
			  0.281064,    0.645281,    -0.167313,    0.685935,    -0.160711,    -0.113289,
			  1.08453,     -0.0970135,  -0.3655,      -0.51894,    0.275308,     -0.000830889,
			  -0.0431051,  0.574405,    -0.163071,    -0.30989,    0.372959,     -0.0161521,
			  0.131741,    0.456781,    0.0165477,    -0.0975113,  -0.273682,    -0.509164,
			  0.573244,    -0.714618,   -0.479023,    0.0525875,   0.316595,     -0.148211,
			  -0.423713,   -0.22462,    -0.528986,    0.390866,    0.0439115,    -0.274567,
			  0.106133,    -0.377686,   0.481055,     0.398664,    0.314325,     0.839894,
			  -0.625382,   0.0543475,   -0.201899,    0.198677,    0.0182834,    0.621111,
			  0.128773,    -0.265686,   0.602337,     0.296946,    0.773769,     0.0479956,
			  -0.132997,   -0.0410526,  -0.254838,    0.326185,    0.347585,     -0.580061,
			  0.405482,    0.101755,    -0.201249,    0.306534,    0.469578,     -0.111657,
			  -0.796765,   -0.0773768,  -0.538891,    0.206104,    -0.0794146,   0.098465,
			  0.413728,    0.0259771,   -0.823897,    0.0925169,   0.88273,      -0.184931,
			  -0.134422,   -0.247737,   -0.682095,    0.177566,    0.299386,     -0.329205,
			  0.0488276,   0.504052,    0.268825,     0.395508,    -1.10225,     0.101069,
			  -0.0408943,  -0.580797,   -0.00804806,  -0.402047,   -0.418787,    0.697977,
			  -0.308492,   -0.122199,   0.628944,     0.54588,     0.0622768,    -0.488552,
			  0.0474367,   0.215963,    -0.679212,    0.311237,    -0.000920773, -0.721814,
			  0.579613,    -0.0458724,  -0.467233,    0.268248,    0.246741,     -0.15576,
			  0.0473638,   0.0246596,   -0.572414,    -0.419131,   -0.357526,    0.452787,
			  -0.112269,   0.710673,    -0.41551,     0.429337,    0.0882859,    -0.433878,
			  -0.0818105,  -0.180361,   0.36754,      -0.49486,    0.449489,     -0.837214,
			  -1.09047,    0.168766,    -0.163687,    0.256186,    0.633943,     -0.012522,
			  0.631576,    -0.27161,    -0.15392,     -0.471082,   -0.071748,    -0.275351,
			  -0.134404,   0.126987,    -0.478438,    -0.144772,   -0.38336,     0.37449,
			  -0.458729,   -0.318997,   -0.313852,    0.081244,    -0.287645,    0.200266,
			  -0.45997,    0.108317,    -0.216842,    -0.165177,   -0.296687,    0.771041,
			  0.933613,    0.617833,    -0.263007,    -0.236543,   -0.406302,    0.241173,
			  -0.225985,   -0.108225,   0.087069,     -0.0444767,  0.645569,     -0.112983,
			  -0.689477,   0.498425,    0.0738087,    0.447277,    0.0972104,    -0.314627,
			  0.393365,    -0.0919185,  -0.32199,     -0.193414,   -0.126091,    0.185217,
			  0.318475,    0.140509,    -0.115877,    -0.911059,   0.336104,     -0.645395,
			  0.00686884,  -0.172296,   -0.513633,    -0.302956,   -1.20699,     0.148284,
			  0.357629,    0.58123,     0.106886,     -0.872183,   -0.49183,     -0.202535,
			  -0.869357,   0.0371933,   -0.0869231,   0.22624,     0.198995,     0.191016,
			  0.151591,    0.347114,    0.056674,     -0.213039,   -0.228541,    -0.473257,
			  -0.574876,   -0.0826995,  -0.730448,    0.343791,    0.795006,     0.366191,
			  0.419235,    -1.11688,    0.227321,     -0.0937171,  0.156708,     -0.3307,
			  0.328026,    -0.454046,   0.432153,     -0.189323,   0.31821,      0.312532,
			  0.0963759,   0.126471,    -0.396326,    0.0353236,   -0.366891,    -0.279321,
			  0.106791,    0.0697961,   0.383726,     0.260039,    0.00297499,   0.45812,
			  -0.544967,   -0.230453,   -0.150821,    -0.374241,   -0.739835,    0.462278,
			  -0.76681,    -0.455701,   0.261229,     0.274824,    0.161605,     -0.402379,
			  0.571192,    0.0844102,   -0.47416,     0.683535,    0.144919,     -0.134556,
			  -0.0414159,  0.357005,    -0.643226,    -0.00324917, -0.173286,    0.770447,
			  0.261563,    0.707628,    0.131681,     0.539707,    -0.367105,    0.150912,
			  -0.310055,   -0.270554,   0.686523,     0.195065,    0.282361,     0.569649,
			  0.106642,    0.296521,    0.185682,     0.124763,    0.182832,     0.42824,
			  -0.489455,   0.55954,     0.383582,     0.52804,     -0.236162,    -0.356153,
			  0.70445,     -0.300133,   1.06101,      0.0289559,   0.4671,       -0.0455821,
			  -1.18106,    0.26797,     0.223324,     0.793996,    -0.833809,    -0.412982,
			  -0.443497,   -0.634181,   -0.000902414, -0.319155,   0.629076,     -0.378669,
			  -0.230422,   0.489184,    0.122302,     0.397895,    0.421496,     -0.41475,
			  0.192182,    -0.477254,   -0.32989,     0.285264,    -0.0248513,   -0.224073,
			  0.520192,    0.138148,    0.783388,     0.540348,    -0.468401,    0.189778,
			  0.327808,    0.387399,    0.0163817,    0.340137,    -0.174623,    -0.560019,
			  -0.32246,    0.353305,    0.513422,     -0.472848,   -0.0151656,   0.0802364,
			  -0.0833406,  0.000303745, -0.359159,    -0.666926,   0.446711,     -0.254889,
			  -0.263977,   0.534997,    0.555322,     -0.315034,   -0.62762,     -0.14342,
			  -0.78082,    0.29739,     0.0783401,    -0.665565,   -0.177726,    0.62018,
			  -0.723053,   0.108446,    0.550657,     0.00324011,  0.387362,     -0.251661,
			  -0.616413,   -0.260163,   -0.798613,    0.0174665,   -0.208833,    -0.0398486,
			  -0.506167,   0.00121689,  -0.75707,     -0.0326216,  0.30282,      0.085227,
			  -0.27267,    0.25662,     0.182456,     -0.184061,   -0.577699,    -0.685311,
			  0.587003,    0.35393,     -0.276868,    -0.0617566,  -0.365888,    0.673723,
			  -0.0476918,  -0.0914235,  0.560627,     -0.387913,   -0.194537,    0.135256,
			  -0.0808623,  0.315394,    -0.0383463,   0.267406,    0.545766,     -0.659403,
			  -0.410556,   0.305285,    0.0364261,    0.396365,    -0.284096,    0.137003,
			  0.611792,    0.191185,    0.440866,     0.87738,     0.470405,     -0.372227,
			  -0.84977,    0.676291,    -0.0709138,   -0.456707,   0.222892,     -0.728947,
			  0.2414,      0.109269,    0.707531,     0.027894,    -0.381266,    -0.1872,
			  -0.674006,   -0.441284,   -0.151681,    -0.695721,   0.360165,     -0.397063,
			  0.02772,     0.271526,    -0.170258,    -0.198509,   0.524165,     0.29589,
			  -0.895699,   -0.266005,   0.0971003,    0.640709,    -0.169635,    0.0263381,
			  -0.779951,   -0.37692,    -0.703577,    0.00526047,  -0.822414,    -0.152364,
			  0.10004,     0.194787,    0.453202,     -0.495236,   1.01192,      -0.682168,
			  -0.453866,   0.387515,    -0.355192,    0.214262,    0.2677,       -0.263514,
			  0.334733,    0.683574,    0.181592,     0.599759,    -0.182972,    0.402297,
			  -0.319075,   0.553958,    -0.990873,    -0.143754,   0.506054,     0.0535431,
			  -0.647583,   0.53928,     -0.510285,    0.452258,    -0.796479,    0.186279,
			  -0.0960782,  -0.124537,   0.509105,     -0.1712,     0.219554,     -0.528307,
			  -0.377211,   -0.447177,   -0.0283537,   0.856948,    -0.128052,    0.482509,
			  0.528981,    -0.785958,   0.816482,     0.213728,    -0.433917,    -0.0413878,
			  -0.997625,   0.228201,    -0.113198,    0.425206,    0.0261474,    0.68678,
			  0.224967,    0.48489,     0.53184,      0.572936,    -0.419627,    -0.70428,
			  -0.216836,   0.57302,     0.640487,     -0.172722,   0.237492,     -0.390903,
			  0.0717416,   0.852097,    -0.0422118,   0.151465,    -0.638427,    0.132246,
			  -0.0552788,  0.436714,    -0.281931,    0.411517,    -0.340499,    -0.725834,
			  -0.478547,   0.332275,    -0.0243354,   -0.499295,   0.238681,     -0.324647,
			  -0.182754,   0.520306,    -0.0762625,   0.631812,    -0.652095,    -0.504378,
			  -0.534564,   0.118165,    -0.384134,    0.611485,    0.635868,     0.100705,
			  0.25619,     0.197184,    0.328731,     -0.0750947,  -0.763023,    0.516191,
			  0.375317,    -0.17778,    0.880709,     0.668956,    0.376694,     0.425053,
			  -0.930982,   0.0534644,   -0.0423658,   0.695356,    0.352989,     0.0400925,
			  0.383482,    0.188746,    0.0193305,    0.128885,    -0.23603,     -0.288163,
			  -0.311799,   -0.425027,   -0.297739,    -0.349681,   -0.278894,    0.00934887,
			  -0.38221,    0.542819,    0.234533,     -0.213422,   0.198418,     0.694582,
			  -0.43395,    -0.417672,   0.553686,     -0.10748,    -0.352711,    -0.0115025,
			  0.0581546,   0.962054,    0.210576,     0.339536,    -0.0818458,   -0.358587,
			  -0.342001,   -0.0689676,  0.0470595,    -0.3791,     0.212149,     -0.00608754,
			  0.318279,    0.246769,    0.514428,     0.457749,    0.759536,     0.236433,
			  0.422228,    0.571146,    -0.247402,    0.667306,    -0.558038,    -0.158556,
			  -0.369374,   -0.341798,   0.30697,      -0.535024,   -0.487844,    -0.0888073,
			  0.404439,    -0.580029,   0.457389,     0.297961,    -0.0356712,   0.508803,
			  0.325652,    -0.239089,   -0.743984,    0.21902,     0.455838,     0.149938,
			  -0.150058,   0.342239,    0.147549,     -0.044282,   -0.634129,    0.266822,
			  -0.764306,   -0.13691,    -0.59542,     -0.503302,   -0.581097,    0.455914,
			  0.193022,    -0.255091,   0.0782733,    0.354385,    0.181455,     -0.579845,
			  -0.597151,   -0.747541,   -0.471478,    -0.257622,   0.80429,      0.908564,
			  0.11331,     -0.210526,   0.893246,     -0.354708,   -0.581153,    0.366957,
			  0.000682831, 1.05443,     0.310998,     0.455284,    -0.251732,    -0.567471,
			  -0.660306,   -0.202108,   0.836359,     -0.467352,   -0.20453,     0.0710459,
			  0.0628843,   -0.132979,   -0.755594,    0.0600963,   0.725805,     -0.221625,
			  0.133578,    -0.802764,   0.00850201,   0.748137,    -0.411616,    -0.136451,
			  0.0531707,   -0.977616,   0.162951,     0.0394506,   -0.0480862,   0.797194,
			  0.52012,     0.238174,    0.169073,     0.249234,    0.00133944,   -0.01138,
			  0.107195,    0.0101681,   -0.247766,    -0.415877,   -0.450288,    0.800731};
			jitterTableIndex += 2;
			if (jitterTableIndex == jitterTable.size()) {
				jitterTableIndex = 0;
			}
			return Vector2{jitterTable[jitterTableIndex], jitterTable[jitterTableIndex + 1]};
		}
	} // namespace draw
} // namespace spades
