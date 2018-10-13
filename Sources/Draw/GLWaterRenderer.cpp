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

#include <cstdlib>
#include <vector>

#include <kiss_fft130/kiss_fft.h>

#include <Client/GameMap.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include "GLFramebufferManager.h"
#include "GLImage.h"
#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "GLShadowShader.h"
#include "GLWaterRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {

#pragma mark - Wave Tank Simulation

		class GLWaterRenderer::IWaveTank : public ConcurrentDispatch {
		protected:
			float dt;
			int size, samples;

		private:
			uint32_t *bitmap;

			int Encode8bit(float v) {
				v = (v + 1.f) * .5f * 255.f;
				v = floorf(v + .5f);

				int i = (int)v;
				if (i < 0)
					i = 0;
				if (i > 255)
					i = 255;
				return i;
			}

			uint32_t MakeBitmapPixel(float dx, float dy, float h) {
				float x = dx, y = dy, z = 0.04f;
				float scale = 200.f;
				x *= scale;
				y *= scale;
				z *= scale;

				uint32_t out;
				out = Encode8bit(z);
				out |= Encode8bit(y) << 8;
				out |= Encode8bit(x) << 16;
				out |= Encode8bit(h * -10.f) << 24;
				return out;
			}

			void MakeBitmapRow(float *h1, float *h2, float *h3, uint32_t *out) {
				out[0] = MakeBitmapPixel(h2[1] - h2[size - 1], h3[0] - h1[0], h2[0]);
				out[size - 1] =
				  MakeBitmapPixel(h2[0] - h2[size - 2], h3[size - 1] - h1[size - 1], h2[size - 1]);
				for (int x = 1; x < size - 1; x++) {
					out[x] = MakeBitmapPixel(h2[x + 1] - h2[x - 1], h3[x] - h1[x], h2[x]);
				}
			}

		public:
			IWaveTank(int size) : size(size) {

				bitmap = new uint32_t[size * size];

				samples = size * size;
			}
			virtual ~IWaveTank() { delete[] bitmap; }
			void SetTimeStep(float dt) { this->dt = dt; }

			int GetSize() const { return size; }

			uint32_t *GetBitmap() const { return bitmap; }

			void MakeBitmap(float *height) {
				MakeBitmapRow(height + (size - 1) * size, height, height + size, bitmap);
				MakeBitmapRow(height + (size - 2) * size, height + (size - 1) * size, height,
				              bitmap + (size - 1) * size);
				for (int y = 1; y < size - 1; y++) {
					MakeBitmapRow(height + (y - 1) * size, height + y * size,
					              height + (y + 1) * size, bitmap + y * size);
				}
			}
		};

#pragma mark - FFT Wave Solver

		struct SinCosTable {
			float sinCoarse[256];
			float cosCoarse[256];
			float sinFine[256];
			float cosFine[256];

		public:
			SinCosTable() {
				for (int i = 0; i < 256; i++) {
					float ang = (float)i / 256.f * (float)M_PI * 2.f;
					sinCoarse[i] = sinf(ang);
					cosCoarse[i] = cosf(ang);

					ang = (float)i / 65536.f * (float)M_PI * 2.f;
					sinFine[i] = sinf(ang);
					cosFine[i] = cosf(ang);
				}
			}

			void Compute(unsigned int step, float &outSin, float &outCos) {
				step &= 0xffff;
				if (step == 0) {
					outSin = 0;
					outCos = 1.f;
					return;
				}

				int fine = step & 0xff;
				int coarse = step >> 8;

				outSin = sinCoarse[coarse];
				outCos = cosCoarse[coarse];

				if (fine != 0) {
					float c = cosFine[fine];
					float s = sinFine[fine];
					float c2 = outCos * c - outSin * s;
					float s2 = outCos * s + outSin * c;
					outCos = c2;
					outSin = s2;
				}
			}
		};

		static SinCosTable sinCosTable;

		template <int SizeBits> class GLWaterRenderer::FFTWaveTank : public IWaveTank {
			enum { Size = 1 << SizeBits, SizeHalf = Size / 2 };
			kiss_fft_cfg fft;

			typedef kiss_fft_cpx Complex;

			struct Cell {
				float magnitude;
				uint32_t phase;
				float phasePerSecond;

				float m00, m01;
				float m10, m11;
			};

			Cell cells[SizeHalf + 1][Size];

			Complex spectrum[SizeHalf + 1][Size];

			Complex temp1[Size];
			Complex temp2[Size];
			Complex temp3[Size][Size];

			float height[Size][Size];

		public:
			FFTWaveTank() : IWaveTank(Size) {
				auto *getRandom = SampleRandomFloat;

				fft = kiss_fft_alloc(Size, 1, NULL, NULL);

				for (int x = 0; x < Size; x++) {
					for (int y = 0; y <= SizeHalf; y++) {
						Cell &cell = cells[y][x];
						if (x == 0 && y == 0) {
							cell.magnitude = 0;
							cell.phasePerSecond = 0.f;
							cell.phase = 0;
						} else {
							int cx = std::min(x, Size - x);
							float dist = (float)sqrtf(cx * cx + y * y);
							float mag = 0.8f / dist / (float)Size;
							mag /= dist;

							float scal = dist / (float)SizeHalf;
							scal *= scal;
							mag *= expf(-scal * 3.f);

							cell.magnitude = mag;
							cell.phase = static_cast<uint32_t>(SampleRandom());
							cell.phasePerSecond = dist * 1.e+9f * 128 / Size;
						}

						cell.m00 = getRandom() - getRandom();
						cell.m01 = getRandom() - getRandom();
						cell.m10 = getRandom() - getRandom();
						cell.m11 = getRandom() - getRandom();
					}
				}
			}
			~FFTWaveTank() { kiss_fft_free(fft); }

			void Run() override {
				// advance cells
				for (int x = 0; x < Size; x++) {
					for (int y = 0; y <= SizeHalf; y++) {
						Cell &cell = cells[y][x];
						uint32_t dphase;
						dphase = (uint32_t)(cell.phasePerSecond * dt);
						cell.phase += dphase;

						unsigned int phase = cell.phase >> 16;
						float c, s;
						sinCosTable.Compute(phase, s, c);

						float u, v;
						u = c * cell.m00 + s * cell.m01;
						v = c * cell.m10 + s * cell.m11;

						spectrum[y][x].r = u * cell.magnitude;
						spectrum[y][x].i = v * cell.magnitude;
					}
				}

				// rfft
				for (int y = 0; y <= SizeHalf; y++) {
					for (int x = 0; x < Size; x++)
						temp1[x] = spectrum[y][x];

					kiss_fft(fft, temp1, temp2);

					if (y == 0) {
						for (int x = 0; x < Size; x++) {
							temp3[x][0] = temp2[x];
						}
					} else if (y == SizeHalf) {
						for (int x = 0; x < Size; x++) {
							temp3[x][SizeHalf].r = temp2[x].r;
							temp3[x][SizeHalf].i = 0.f;
						}
					} else {
						for (int x = 0; x < Size; x++) {
							temp3[x][y] = temp2[x];
							temp3[x][Size - y].r = temp2[x].r;
							temp3[x][Size - y].i = -temp2[x].i;
						}
					}
				}
				for (int x = 0; x < Size; x++) {
					kiss_fft(fft, temp3[x], temp2);
					for (int y = 0; y < Size; y++) {
						height[x][y] = temp2[y].r;
					}
				}

				MakeBitmap((float *)height);
			}
		};

#pragma mark - FTCS PDE Solver

		class GLWaterRenderer::StandardWaveTank : public IWaveTank {
			float *height;
			float *heightFiltered;
			float *velocity;

			template <bool xy> void DoPDELine(float *vy, float *y1, float *y2, float *yy) {
				int pitch = xy ? size : 1;
				for (int i = 0; i < size; i++) {
					float v1 = *y1, v2 = *y2, v = *yy;
					float force = v1 + v2 - (v + v);
					force *= dt * 80.f;
					*vy += force;

					y1 += pitch;
					y2 += pitch;
					yy += pitch;
					vy += pitch;
				}
			}

			template <bool xy> void Denoise(float *arr) {
				int pitch = xy ? size : 1;
#if 1
				if ((arr[0] > 0.f && arr[(size - 1) * pitch] < 0.f && arr[pitch] < 0.f) ||
				    (arr[0] < 0.f && arr[(size - 1) * pitch] > 0.f && arr[pitch] > 0.f)) {
					float ttl = (arr[1] + arr[(size - 1) * pitch]) * .5f;
					arr[0] = ttl;
				}
				if ((arr[(size - 1) * pitch] > 0.f && arr[(size - 2) * pitch] < 0.f &&
				     arr[0] < 0.f) ||
				    (arr[(size - 1) * pitch] < 0.f && arr[(size - 2) * pitch] > 0.f &&
				     arr[0] > 0.f)) {
					float ttl = (arr[0] + arr[(size - 2) * pitch]) * .5f;
					arr[(size - 1) * pitch] = ttl;
				}
				for (int i = 1; i < size - 1; i++) {
					if ((arr[i * pitch] > 0.f && arr[(i - 1) * pitch] < 0.f &&
					     arr[(i + 1) * pitch] < 0.f) ||
					    (arr[i * pitch] < 0.f && arr[(i - 1) * pitch] > 0.f &&
					     arr[(i + 1) * pitch] > 0.f)) {
						float ttl = (arr[(i + 1) * pitch] + arr[(i - 1) * pitch]) * .5f;
						arr[i * pitch] = ttl;
					}
				}
#else
				// Lax-Friedrich
				float buf[256]; // TODO: variable size
				SPAssert(size <= 256);
				for (int i = 0; i < size; i++)
					buf[i] = arr[i * pitch] * .5f;

				arr[0] = buf[1] + buf[size - 1];
				arr[(size - 1) * pitch] = buf[size - 2] + buf[0];

				for (int i = 1; i < size - 1; i++)
					arr[i * pitch] = buf[i - 1] + buf[i + 1];

#endif
			}

		public:
			StandardWaveTank(int size) : IWaveTank(size) {
				height = new float[size * size];
				heightFiltered = new float[size * size];
				velocity = new float[size * size];
				std::fill(height, height + size * size, 0.f);
				std::fill(velocity, velocity + size * size, 0.f);
			}

			~StandardWaveTank() {

				delete[] height;
				delete[] heightFiltered;
				delete[] velocity;
			}

			void Run() override {
				// advance time
				for (int i = 0; i < samples; i++)
					height[i] += velocity[i] * dt;
#ifndef NDEBUG
				for (int i = 0; i < samples; i++)
					SPAssert(!std::isnan(height[i]));
				for (int i = 0; i < samples; i++)
					SPAssert(!std::isnan(velocity[i]));
#endif

				// solve ddz/dtt = c^2 (ddz/dxx + ddz/dyy)

				// do ddz/dyy
				DoPDELine<false>(velocity, height + (size - 1) * size, height + size, height);
				DoPDELine<false>(velocity + (size - 1) * size, height + (size - 2) * size, height,
				                 height + (size - 1) * size);
				for (int y = 1; y < size - 1; y++) {
					DoPDELine<false>(velocity + y * size, height + (y - 1) * size,
					                 height + (y + 1) * size, height + y * size);
				}

				// do ddz/dxx
				DoPDELine<true>(velocity, height + (size - 1), height + 1, height);
				DoPDELine<true>(velocity + (size - 1), height + (size - 2), height,
				                height + (size - 1));
				for (int x = 1; x < size - 1; x++) {
					DoPDELine<true>(velocity + x, height + (x - 1), height + (x + 1), height + x);
				}

				// make average 0
				float sum = 0.f;
				for (int i = 0; i < samples; i++)
					sum += height[i];
				sum /= (float)samples;
				for (int i = 0; i < samples; i++)
					height[i] -= sum;

				// limit energy
				sum = 0.f;
				for (int i = 0; i < samples; i++) {
					sum += height[i] * height[i];
					sum += velocity[i] * velocity[i];
				}
				sum = sqrtf(sum / (float)samples / 2.f) * 80.f;
				if (sum > 1.f) {
					sum = 1.f / sum;
					for (int i = 0; i < samples; i++) {
						height[i] *= sum;
						velocity[i] *= sum;
					}
				}

				// denoise
				for (int i = 0; i < size; i++) {
					Denoise<true>(height + i);
				}
				for (int i = 0; i < size; i++) {
					Denoise<false>(height + i * size);
				}

				// add randomness
				int count = (int)floorf(dt * 600.f);
				if (count > 400)
					count = 400;
				
				for (int i = 0; i < count; i++) {
					int ox = SampleRandomInt(0, size - 3);
					int oy = SampleRandomInt(0, size - 3);
					static const float gauss[] = {0.225610111284052f, 0.548779777431897f,
					                              0.225610111284052f};
					float strength = (SampleRandomFloat() - SampleRandomFloat()) * 0.15f * 100.f;
					for (int x = 0; x < 3; x++)
						for (int y = 0; y < 3; y++) {
							velocity[(x + ox) + (y + oy) * size] += strength * gauss[x] * gauss[y];
						}
				}

				for (int i = 0; i < samples; i++)
					heightFiltered[i] = height[i]; // * height[i] * 100.f;

				// build bitmap
				MakeBitmap(heightFiltered);
			}
		};

#pragma mark - Water Renderer

		void GLWaterRenderer::PreloadShaders(spades::draw::GLRenderer *renderer) {
			auto &settings = renderer->GetSettings();
			if ((int)settings.r_water >= 3)
				renderer->RegisterProgram("Shaders/Water3.program");
			else if ((int)settings.r_water >= 2)
				renderer->RegisterProgram("Shaders/Water2.program");
			else
				renderer->RegisterProgram("Shaders/Water.program");
		}

		GLWaterRenderer::GLWaterRenderer(GLRenderer *renderer, client::GameMap *map)
		    : renderer(renderer),
		      device(renderer->GetGLDevice()),
		      settings(renderer->GetSettings()),
		      map(map) {
			SPADES_MARK_FUNCTION();
			if ((int)settings.r_water >= 3)
				program = renderer->RegisterProgram("Shaders/Water3.program");
			else if ((int)settings.r_water >= 2)
				program = renderer->RegisterProgram("Shaders/Water2.program");
			else
				program = renderer->RegisterProgram("Shaders/Water.program");
			BuildVertices();

			tempDepthTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, tempDepthTexture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::DepthComponent24,
			                   device->ScreenWidth(), device->ScreenHeight(), 0,
			                   IGLDevice::DepthComponent, IGLDevice::UnsignedInt, NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                     IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                     IGLDevice::ClampToEdge);

			tempFramebuffer = device->GenFramebuffer();
			device->BindFramebuffer(IGLDevice::Framebuffer, tempFramebuffer);
			device->FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
			                             IGLDevice::Texture2D, tempDepthTexture, 0);

			// create water color texture
			texture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, texture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8, map->Width(),
			                   map->Height(), 0, IGLDevice::RGBA, IGLDevice::UnsignedByte, NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::Repeat);

			w = map->Width();
			h = map->Height();

			updateBitmapPitch = (w + 31) / 32;
			updateBitmap.resize(updateBitmapPitch * h);

			bitmap.resize(w * h);
			std::fill(updateBitmap.begin(), updateBitmap.end(), 0xffffffffUL);
			std::fill(bitmap.begin(), bitmap.end(), 0xffffffffUL);

			device->TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, w, h, IGLDevice::BGRA,
			                      IGLDevice::UnsignedByte, bitmap.data());


			size_t numLayers = ((int)settings.r_water >= 2) ? 3 : 1;


			// create wave tank simlation
			for (size_t i = 0; i < numLayers; i++) {
				if ((int)settings.r_water >= 3) {
					waveTanks.push_back(new FFTWaveTank<8>());
				} else {
					waveTanks.push_back(new FFTWaveTank<7>());
				}
			}
			
			// create heightmap texture
			waveTexture = device->GenTexture();
			if (numLayers == 1) {
				device->BindTexture(IGLDevice::Texture2D, waveTexture);
				device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8,
				                   waveTanks[0]->GetSize(), waveTanks[0]->GetSize(), 0,
				                   IGLDevice::BGRA, IGLDevice::UnsignedByte, NULL);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                     IGLDevice::LinearMipmapLinear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                     IGLDevice::Repeat);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                     IGLDevice::Repeat);
				if (settings.r_maxAnisotropy > 1.0f) {
					device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMaxAnisotropy,
					                     (float)settings.r_maxAnisotropy);
				}
			} else {
				device->BindTexture(IGLDevice::Texture2DArray, waveTexture);
				device->TexImage3D(IGLDevice::Texture2DArray, 0, IGLDevice::RGBA8,
				                   waveTanks[0]->GetSize(), waveTanks[0]->GetSize(), static_cast<IGLDevice::Sizei>(numLayers), 0,
				                   IGLDevice::BGRA, IGLDevice::UnsignedByte, NULL);
				device->TexParamater(IGLDevice::Texture2DArray, IGLDevice::TextureMagFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2DArray, IGLDevice::TextureMinFilter,
				                     IGLDevice::LinearMipmapLinear);
				device->TexParamater(IGLDevice::Texture2DArray, IGLDevice::TextureWrapS,
				                     IGLDevice::Repeat);
				device->TexParamater(IGLDevice::Texture2DArray, IGLDevice::TextureWrapT,
				                     IGLDevice::Repeat);
				if (settings.r_maxAnisotropy > 1.0f) {
					device->TexParamater(IGLDevice::Texture2DArray, IGLDevice::TextureMaxAnisotropy,
					                     (float)settings.r_maxAnisotropy);
				}
			}

			occlusionQuery = 0;
		}

		struct GLWaterRenderer::Vertex {
			float x, y;
		};

		void GLWaterRenderer::BuildVertices() {
			SPADES_MARK_FUNCTION();
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			int meshSize = 16;
			if ((int)settings.r_water >= 2)
				meshSize = 128;
			float meshSizeInv = 1.f / (float)meshSize;
			for (int y = -meshSize; y <= meshSize; y++) {
				for (int x = -meshSize; x <= meshSize; x++) {
					Vertex v;
					v.x = (float)(x)*meshSizeInv;
					v.y = (float)(y)*meshSizeInv;

					// higher density near the camera
					v.x *= v.x * v.x;
					v.y *= v.y * v.y;

					vertices.push_back(v);
				}
			}
#define VID(x, y) (((x) + meshSize) + ((y) + meshSize) * (meshSize * 2 + 1))
			for (int x = -meshSize; x < meshSize; x++) {
				for (int y = -meshSize; y < meshSize; y++) {
					indices.push_back(VID(x, y));
					indices.push_back(VID(x + 1, y));
					indices.push_back(VID(x, y + 1));

					indices.push_back(VID(x + 1, y));
					indices.push_back(VID(x + 1, y + 1));
					indices.push_back(VID(x, y + 1));
				}
			}

			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(sizeof(Vertex) * vertices.size()),
			                   vertices.data(), IGLDevice::StaticDraw);
			idxBuffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, idxBuffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(sizeof(uint32_t) * indices.size()),
			                   indices.data(), IGLDevice::StaticDraw);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			numIndices = indices.size();
		}

		GLWaterRenderer::~GLWaterRenderer() {
			SPADES_MARK_FUNCTION();
			device->DeleteBuffer(buffer);
			device->DeleteBuffer(idxBuffer);
			device->DeleteFramebuffer(tempFramebuffer);
			device->DeleteTexture(tempDepthTexture);
			device->DeleteTexture(texture);

			if (occlusionQuery)
				device->DeleteQuery(occlusionQuery);

			for (size_t i = 0; i < waveTanks.size(); i++) {
				waveTanks[i]->Join();
				delete waveTanks[i];
			}
			device->DeleteTexture(waveTexture);
		}

		void GLWaterRenderer::Render() {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer->GetGLProfiler(), "Render");

			if (occlusionQuery == 0 && settings.r_occlusionQuery)
				occlusionQuery = device->GenQuery();

			GLColorBuffer colorBuffer;

			{
				GLProfiler::Context profiler(renderer->GetGLProfiler(), "Preparation");
				colorBuffer = renderer->GetFramebufferManager()->PrepareForWaterRendering(
				  tempFramebuffer, tempDepthTexture);
			}

			float fogDist = renderer->GetFogDistance();
			Vector3 fogCol = renderer->GetFogColorForSolidPass();
			fogCol *= fogCol; // linearize

			Vector3 skyCol = renderer->GetFogColor();
			skyCol *= skyCol; // linearize

			const client::SceneDefinition &def = renderer->GetSceneDef();
			float waterLevel = 63.f;
			float waterRange = 128.f;

			Matrix4 mat = Matrix4::Translate(def.viewOrigin.x, def.viewOrigin.y, waterLevel);
			mat = mat * Matrix4::Scale(waterRange, waterRange, 1.f);

			GLProfiler::Context profiler2(renderer->GetGLProfiler(), "Draw Plane");

			// do color
			device->DepthFunc(IGLDevice::Less);
			device->ColorMask(true, true, true, true);
			{
				GLProgram *prg = program;
				prg->Use();

				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				static GLProgramUniform projectionViewMatrix("projectionViewMatrix");
				static GLProgramUniform modelMatrix("modelMatrix");
				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				static GLProgramUniform viewMatrix("viewMatrix");
				static GLProgramUniform fogDistance("fogDistance");
				static GLProgramUniform fogColor("fogColor");
				static GLProgramUniform skyColor("skyColor");
				static GLProgramUniform zNearFar("zNearFar");
				static GLProgramUniform viewOrigin("viewOrigin");
				static GLProgramUniform displaceScale("displaceScale");
				static GLProgramUniform fovTan("fovTan");
				static GLProgramUniform waterPlane("waterPlane");

				projectionViewModelMatrix(prg);
				projectionViewMatrix(prg);
				modelMatrix(prg);
				viewModelMatrix(prg);
				viewMatrix(prg);
				fogDistance(prg);
				fogColor(prg);
				skyColor(prg);
				zNearFar(prg);
				viewOrigin(prg);
				displaceScale(prg);
				fovTan(prg);
				waterPlane(prg);

				projectionViewModelMatrix.SetValue(renderer->GetProjectionViewMatrix() * mat);
				projectionViewMatrix.SetValue(renderer->GetProjectionViewMatrix());
				modelMatrix.SetValue(mat);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * mat);
				viewMatrix.SetValue(renderer->GetViewMatrix());
				fogDistance.SetValue(fogDist);
				fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);
				skyColor.SetValue(skyCol.x, skyCol.y, skyCol.z);
				zNearFar.SetValue(def.zNear, def.zFar);
				viewOrigin.SetValue(def.viewOrigin.x, def.viewOrigin.y, def.viewOrigin.z);
				/*displaceScale.SetValue(1.f / renderer->ScreenWidth() / tanf(def.fovX * .5f),
				                       1.f / renderer->ScreenHeight() / tanf(def.fovY) * .5f);*/
				displaceScale.SetValue(1.f / tanf(def.fovX * .5f), 1.f / tanf(def.fovY * .5f));
				fovTan.SetValue(tanf(def.fovX * .5f), -tanf(def.fovY * .5f), -tanf(def.fovX * .5f),
				                tanf(def.fovY * .5f));

				// make water plane in view coord
				Matrix4 wmat = renderer->GetViewMatrix() * mat;
				Vector3 dir = wmat.GetAxis(2);
				waterPlane.SetValue(dir.x, dir.y, dir.z, -Vector3::Dot(dir, wmat.GetOrigin()));

				static GLProgramUniform screenTexture("screenTexture");
				static GLProgramUniform depthTexture("depthTexture");
				static GLProgramUniform textureUnif("mainTexture");
				static GLProgramUniform waveTextureUnif("waveTexture");
				static GLProgramUniform waveTextureArrayUnif("waveTextureArray");
				static GLProgramUniform mirrorTexture("mirrorTexture");
				static GLProgramUniform mirrorDepthTexture("mirrorDepthTexture");

				screenTexture(prg);
				depthTexture(prg);
				textureUnif(prg);
				waveTextureUnif(prg);
				waveTextureArrayUnif(prg);
				mirrorTexture(prg);
				mirrorDepthTexture(prg);

				device->ActiveTexture(0);
				device->BindTexture(IGLDevice::Texture2D, colorBuffer.GetTexture());
				screenTexture.SetValue(0);
				// depth is not interpolated, so color shouldn't be
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                     IGLDevice::Nearest);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                     IGLDevice::Nearest);

				device->ActiveTexture(1);
				device->BindTexture(IGLDevice::Texture2D, tempDepthTexture);
				depthTexture.SetValue(1);

				device->ActiveTexture(2);
				device->BindTexture(IGLDevice::Texture2D, texture);
				textureUnif.SetValue(2);

				static GLShadowShader shadowShader;

				if (waveTanks.size() == 1) {
					device->ActiveTexture(3);
					device->BindTexture(IGLDevice::Texture2D, waveTexture);
					waveTextureUnif.SetValue(3);

					shadowShader(renderer, prg, 4);
				} else if (waveTanks.size() == 3) {
					device->ActiveTexture(3);
					device->BindTexture(IGLDevice::Texture2DArray, waveTexture);
					waveTextureArrayUnif.SetValue(3);

					// mirror
					device->ActiveTexture(4);
					device->BindTexture(IGLDevice::Texture2D,
					                    renderer->GetFramebufferManager()->GetMirrorTexture());
					if ((float)settings.r_maxAnisotropy > 1.1f) {
						device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMaxAnisotropy,
						                     (float)settings.r_maxAnisotropy);
					}
					mirrorTexture.SetValue(4);

					if ((int)settings.r_water >= 3) {
						device->ActiveTexture(5);
						device->BindTexture(
						  IGLDevice::Texture2D,
						  renderer->GetFramebufferManager()->GetMirrorDepthTexture());
						mirrorDepthTexture.SetValue(5);

						shadowShader(renderer, prg, 6);
					} else {
						shadowShader(renderer, prg, 5);
					}
				} else {
					SPAssert(false);
				}

				static GLProgramAttribute positionAttribute("positionAttribute");

				positionAttribute(prg);

				device->EnableVertexAttribArray(positionAttribute(), true);

				device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
				device->VertexAttribPointer(positionAttribute(), 2, IGLDevice::FloatType, false,
				                            sizeof(Vertex), NULL);
				device->BindBuffer(IGLDevice::ArrayBuffer, 0);

				device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

				if (occlusionQuery)
					device->BeginQuery(IGLDevice::SamplesPassed, occlusionQuery);

				device->DrawElements(IGLDevice::Triangles,
				                     static_cast<IGLDevice::Sizei>(numIndices),
				                     IGLDevice::UnsignedInt, NULL);

				if (occlusionQuery)
					device->EndQuery(IGLDevice::SamplesPassed);

				device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

				device->EnableVertexAttribArray(positionAttribute(), false);

				device->ActiveTexture(0);
				// restore filter mode for color buffer
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                     IGLDevice::Linear);
			}
		}

		static uint32_t LinearlizeColor(uint32_t v) {
			int r = (uint8_t)(v);
			int g = (uint8_t)(v >> 8);
			int b = (uint8_t)(v >> 16);
			r = (r * r + 128) >> 8;
			g = (g * g + 128) >> 8;
			b = (b * b + 128) >> 8;
			return b | (g << 8) | (r << 16);
		}

		void GLWaterRenderer::Update(float dt) {
			SPADES_MARK_FUNCTION();
			GLProfiler::Context profiler(renderer->GetGLProfiler(), "Update");

			// update wavetank simulation
			{
				GLProfiler::Context profiler(renderer->GetGLProfiler(), "Waiting for Simulation To Done");
				for (size_t i = 0; i < waveTanks.size(); i++) {
					waveTanks[i]->Join();
				}
			}
			{
				{
					GLProfiler::Context profiler(renderer->GetGLProfiler(), "Upload");
					if (waveTanks.size() == 1) {
						device->BindTexture(IGLDevice::Texture2D, waveTexture);
						device->TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0,
						                      waveTanks[0]->GetSize(), waveTanks[0]->GetSize(),
						                      IGLDevice::BGRA, IGLDevice::UnsignedByte,
						                      waveTanks[0]->GetBitmap());
					} else {
						device->BindTexture(IGLDevice::Texture2DArray, waveTexture);
						for (size_t i = 0; i < waveTanks.size(); i++) {
							device->TexSubImage3D(IGLDevice::Texture2DArray, 0, 0, 0, static_cast<IGLDevice::Sizei>(i),
												  waveTanks[i]->GetSize(), waveTanks[i]->GetSize(), 1,
												  IGLDevice::BGRA, IGLDevice::UnsignedByte,
												  waveTanks[i]->GetBitmap());
						}
					}
				}
				{
					GLProfiler::Context profiler(renderer->GetGLProfiler(), "Generate Mipmap");
					if (waveTanks.size() == 1) {
						device->BindTexture(IGLDevice::Texture2D, waveTexture);
						device->GenerateMipmap(IGLDevice::Texture2D);
					} else {
						device->BindTexture(IGLDevice::Texture2DArray, waveTexture);
						device->GenerateMipmap(IGLDevice::Texture2DArray);
					}
				}
			}
			for (size_t i = 0; i < waveTanks.size(); i++) {
				switch (i) {
					case 0: waveTanks[i]->SetTimeStep(dt); break;
					case 1: waveTanks[i]->SetTimeStep(dt * 0.15704f / .08f); break;
					case 2: waveTanks[i]->SetTimeStep(dt * 0.02344f / .08f); break;
				}
				waveTanks[i]->Start();
			}

			{
				GLProfiler::Context profiler(renderer->GetGLProfiler(), "Upload Water Color Texture");
				device->BindTexture(IGLDevice::Texture2D, texture);
				bool fullUpdate = true;
				for (size_t i = 0; i < updateBitmap.size(); i++) {
					if (updateBitmap[i] == 0) {
						fullUpdate = false;
						break;
					}
				}

				if (fullUpdate) {
					uint32_t *pixels = bitmap.data();
					bool modified = false;
					int x = 0, y = 0;
					for (int i = w * h; i > 0; i--) {
						uint32_t col = map->GetColor(x, y, 63);

						x++;
						if (x == w) {
							x = 0;
							y++;
						}

						col = LinearlizeColor(col);

						if (*pixels != col)
							modified = true;
						else {
							pixels++;
							continue;
						}
						*(pixels++) = col;
					}

					if (modified) {
						device->TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, w, h, IGLDevice::BGRA,
						                      IGLDevice::UnsignedByte, bitmap.data());
					}

					for (size_t i = 0; i < updateBitmap.size(); i++) {
						updateBitmap[i] = 0;
					}
				} else {
					// partial update
					for (size_t i = 0; i < updateBitmap.size(); i++) {
						int y = static_cast<int>(i / updateBitmapPitch);
						int x = static_cast<int>((i - y * updateBitmapPitch) * 32);
						if (updateBitmap[i] == 0)
							continue;

						uint32_t *pixels = bitmap.data() + x + y * w;
						bool modified = false;
						for (int j = 0; j < 32; j++) {
							uint32_t col = map->GetColor(x + j, y, 63);

							col = LinearlizeColor(col);

							if (pixels[j] != col)
								modified = true;
							else
								continue;
							pixels[j] = col;
							// pixels[j] = GeneratePixel(x + j, y);
						}

						if (modified) {
							device->TexSubImage2D(IGLDevice::Texture2D, 0, x, y, 32, 1,
							                      IGLDevice::BGRA, IGLDevice::UnsignedByte, pixels);
						}

						updateBitmap[i] = 0;
					}
					// partial update - done
				}
			}
		}

		void GLWaterRenderer::MarkUpdate(int x, int y) {
			x &= w - 1;
			y &= h - 1;
			updateBitmap[(x >> 5) + y * updateBitmapPitch] |= 1UL << (x & 31);
		}

		void GLWaterRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			if (map != this->map)
				return;
			if (z < 63)
				return;
			MarkUpdate(x, y);
		}
	}
}
