//
//  GLWaterRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 8/1/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLWaterRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include "GLFramebufferManager.h"
#include <vector>
#include "GLProgramUniform.h"
#include "GLProgramAttribute.h"
#include "GLImage.h"
#include "GLProgram.h"
#include "GLShadowShader.h"
#include "../Core/Debug.h"
#include "../Client/GameMap.h"
#include "../Core/ConcurrentDispatch.h"
#include <stdlib.h>
#include "../kiss_fft130/kiss_fft.h"
#include "GLProfiler.h"

namespace spades {
	namespace draw {
		
#pragma mark - Wave Tank Simulation
		
		class GLWaterRenderer::IWaveTank: public ConcurrentDispatch {
		protected:
			float dt;
			int size, samples;
		private:
			uint32_t *bitmap;
			
						
			int Encode8bit(float v){
				v = (v + 1.f) * .5f * 255.f;
				v = floorf(v + .5f);
				
				int i = (int)v;
				if(i < 0) i = 0;
				if(i > 255) i = 255;
				return i;
			}
			
			uint32_t MakeBitmapPixel(float dx, float dy, float h){
				float x = dx, y = dy, z = 0.02f;
				float len = sqrtf(x*x+y*y+z*z);
				float scale = 1.f / len;
				x *= scale; y *= scale; z *= scale;
				
				uint32_t out;
				out = Encode8bit(z);
				out |= Encode8bit(y) << 8;
				out |= Encode8bit(x) << 16;
				out |= Encode8bit(h) << 24;
				return out;
			}
			
			void MakeBitmapRow(float *h1, float *h2, float *h3,
							   uint32_t *out) {
				out[0] = MakeBitmapPixel(h2[1] - h2[size-1],
										 h3[0]-h1[0],
										 h2[0]);
				out[size-1] = MakeBitmapPixel(h2[0] - h2[size-2],
										 h3[size-1]-h1[size-1],
											  h2[size-1]);
				for(int x = 1; x < size - 1; x++) {
					out[x] = MakeBitmapPixel(h2[x+1]-h2[x-1], h3[x]-h1[x],
											 h2[x]);
				}
			}
			
		public:
			IWaveTank(int size):size(size) {
				
				bitmap = new uint32_t[size*size];
				
				
				samples = size * size;
			}
			virtual ~IWaveTank(){
				delete[] bitmap;
			}
			void SetTimeStep(float dt){
				this->dt = dt;
			}
			
			int GetSize()  const {
				return size;
			}
			
			uint32_t *GetBitmap() const {
				return bitmap;
			}
			
			void MakeBitmap(float *height){
				MakeBitmapRow(height + (size-1) * size,
							  height,
							  height + size,
							  bitmap);
				MakeBitmapRow(height + (size-2) * size,
							  height + (size-1) * size,
							  height,
							  bitmap + (size-1)*size);
				for(int y = 1; y < size - 1; y++){
					MakeBitmapRow(height + (y - 1) * size,
								  height + y * size,
								  height + (y + 1) * size,
								  bitmap + y*size);
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
				for(int i = 0; i < 256; i++){
					float ang = (float)i / 256.f * (float)M_PI * 2.f;
					sinCoarse[i] = sinf(ang);
					cosCoarse[i] = cosf(ang);
					
					ang = (float)i / 65536.f * (float)M_PI * 2.f;
					sinFine[i] = sinf(ang);
					cosFine[i] = cosf(ang);
				}
			}
			
			void Compute(unsigned int step, float& outSin, float &outCos) {
				step &= 0xffff;
				if(step == 0){
					outSin = 0;
					outCos = 1.f;
					return;
				}
				
				int fine = step & 0xff;
				int coarse = step >> 8;
				
				
				outSin = sinCoarse[coarse];
				outCos = cosCoarse[coarse];
				
				if(fine != 0){
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
		
		class GLWaterRenderer::FFTWaveTank: public IWaveTank {
			enum {
				SizeBits = 7,
				Size = 1 << SizeBits,
				SizeHalf = Size / 2
			};
			kiss_fft_cfg fft;
			
			typedef kiss_fft_cpx Complex;
			
			struct Cell {
				float magnitude;
				uint32_t phase;
				float phasePerSecond;
				
				float m00, m01;
				float m10, m11;
			};
			
			Cell cells[SizeHalf+1][Size];
			
			Complex spectrum[SizeHalf+1][Size];
			
			Complex temp1[Size];
			Complex temp2[Size];
			Complex temp3[Size][Size];
			
			float height[Size][Size];
			
		public:
			FFTWaveTank(): IWaveTank(Size){
				fft = kiss_fft_alloc(Size, 1, NULL, NULL);
				
				for(int x = 0; x < Size; x++){
					for(int y = 0; y <= SizeHalf; y++){
						Cell& cell = cells[y][x];
						if(x == 0 && y == 0){
							cell.magnitude = 0;
							cell.phasePerSecond = 0.f;
							cell.phase = 0;
						}else{
							int cx = std::min(x, Size-x);
							float dist = (float)sqrtf(cx*cx+y*y);
							float mag = 0.8f / dist / (float)Size;
							mag /= dist;
							
							if(dist > (float)SizeHalf * .9f)
								mag = 0.f;
							
							cell.magnitude = mag;
							cell.phase = rand() | ((uint32_t)rand() << 16);
							cell.phasePerSecond = dist * 1.e+9f;
						}
						
						cell.m00 = GetRandom() - GetRandom();
						cell.m01 = GetRandom() - GetRandom();
						cell.m10 = GetRandom() - GetRandom();
						cell.m11 = GetRandom() - GetRandom();
					}
				}
			}
			virtual ~FFTWaveTank(){
				kiss_fft_free(fft);
			}
			
			virtual void Run() {
				// advance cells
				for(int x = 0; x < Size; x++){
					for(int y = 0; y <= SizeHalf; y++){
						Cell& cell = cells[y][x];
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
				for(int y = 0; y <= SizeHalf; y++){
					for(int x = 0; x < Size; x++)
						temp1[x] = spectrum[y][x];
					
					kiss_fft(fft, temp1, temp2);
					
					if(y == 0){
						for(int x = 0; x < Size; x++){
							temp3[x][0] = temp2[x];
						}
					}else if(y == SizeHalf){
						for(int x = 0; x < Size; x++){
							temp3[x][SizeHalf].r = temp2[x].r;
							temp3[x][SizeHalf].i = 0.f;
						}
					}else{
						for(int x = 0; x < Size; x++){
							temp3[x][y] = temp2[x];
							temp3[x][Size-y].r = temp2[x].r;
							temp3[x][Size-y].i = -temp2[x].i;
						}
					}
				}
				for(int x = 0; x < Size; x++){
					kiss_fft(fft, temp3[x], temp2);
					for(int y = 0; y < Size; y++){
						height[x][y] = temp2[y].r;
					}
				}
				
				MakeBitmap((float *)height);
				
			}
			
		};
		
#pragma mark - FTCS PDE Solver
		
		class GLWaterRenderer::StandardWaveTank: public IWaveTank {
			float *height;
			float *heightFiltered;
			float *velocity;
			
			template<bool xy>
			void DoPDELine(float *vy, float *y1, float *y2, float *yy) {
				int pitch = xy ? size : 1;
				for(int i = 0; i < size; i++){
					float v1 = *y1, v2 = *y2, v = *yy;
					float force = v1 + v2 - (v+v);
					force *= dt * 80.f;
					*vy += force;
					
					y1 += pitch;
					y2 += pitch;
					yy += pitch;
					vy += pitch;
				}
			}
			
			template<bool xy>
			void Denoise(float *arr) {
				int pitch = xy ? size : 1;
#if 1
				if((arr[0] > 0.f &&
					arr[(size-1)*pitch] < 0.f && arr[pitch] < 0.f) ||
				   (arr[0] < 0.f &&
					arr[(size-1)*pitch] > 0.f && arr[pitch] > 0.f)){
					   float ttl = (arr[1] + arr[(size-1)*pitch]) * .5f;
					   arr[0] = ttl;
				   }
				if((arr[(size-1)*pitch] > 0.f &&
					arr[(size-2)*pitch] < 0.f && arr[0] < 0.f) ||
				   (arr[(size-1)*pitch] < 0.f &&
					arr[(size-2)*pitch] > 0.f && arr[0] > 0.f)){
					   float ttl = (arr[0] + arr[(size-2)*pitch]) * .5f;
					   arr[(size-1)*pitch] = ttl;
				   }
				for(int i = 1 ; i < size - 1; i++){
					if((arr[i*pitch] > 0.f &&
						arr[(i-1)*pitch] < 0.f && arr[(i+1)*pitch] < 0.f) ||
					   (arr[i*pitch] < 0.f &&
						arr[(i-1)*pitch] > 0.f && arr[(i+1)*pitch] > 0.f)){
						   float ttl = (arr[(i+1)*pitch] + arr[(i-1)*pitch]) * .5f;
						   arr[i*pitch] = ttl;
					   }
				}
#else
				//Lax-Friedrich
				float buf[256]; // TODO: variable size
				SPAssert(size <= 256);
				for(int i = 0; i < size; i++)
					buf[i] = arr[i * pitch] * .5f;
				
				arr[0] = buf[1] + buf[size-1];
				arr[(size-1)*pitch] = buf[size-2] + buf[0];
				
				for(int i = 1; i < size - 1; i++)
					arr[i*pitch] = buf[i - 1] + buf[i + 1];
				
#endif
			}

		public:
			
			StandardWaveTank(int size):
			IWaveTank(size){
				height = new float[size*size];
				heightFiltered = new float[size*size];
				velocity = new float[size*size];
				std::fill(height, height + size * size, 0.f);
				std::fill(velocity, velocity + size * size, 0.f);
			}
			
			virtual ~StandardWaveTank(){
				
				delete[] height;
				delete[] heightFiltered;
				delete[] velocity;
			}
			
			virtual void Run() {
				// advance time
				for(int i = 0; i < samples; i++)
					height[i] += velocity[i] * dt;
#ifndef NDEBUG
				for(int i = 0; i < samples; i++)
					SPAssert(!isnan(height[i]));
				for(int i = 0; i < samples; i++)
					SPAssert(!isnan(velocity[i]));
#endif
				
				// solve ddz/dtt = c^2 (ddz/dxx + ddz/dyy)
				
				// do ddz/dyy
				DoPDELine<false>(velocity,
								 height + (size-1)*size,
								 height + size,
								 height);
				DoPDELine<false>(velocity + (size-1)*size,
								 height + (size-2)*size,
								 height,
								 height + (size-1)*size);
				for(int y = 1; y < size - 1; y++) {
					DoPDELine<false>(velocity + y * size,
									 height + (y-1) * size,
									 height + (y+1) * size,
									 height + y * size);
				}
				
				// do ddz/dxx
				DoPDELine<true>(velocity,
								height + (size-1),
								height + 1,
								height);
				DoPDELine<true>(velocity + (size-1),
								height + (size-2),
								height,
								height + (size-1));
				for(int x = 1; x < size - 1; x++) {
					DoPDELine<true>(velocity + x,
									height + (x-1),
									height + (x+1),
									height + x);
				}
				
				// make average 0
				float sum = 0.f;
				for(int i = 0; i < samples; i++)
					sum += height[i];
				sum /= (float)samples;
				for(int i = 0; i < samples; i++)
					height[i] -= sum;
				
				// limit energy
				sum = 0.f;
				for(int i = 0; i < samples; i++){
					sum += height[i] * height[i];
					sum += velocity[i] * velocity[i];
				}
				sum = sqrtf(sum / (float)samples / 2.f) * 80.f;
				if(sum > 1.f){
					sum = 1.f / sum;
					for(int i = 0; i < samples; i++){
						height[i] *= sum;
						velocity[i] *= sum;
					}
				}
				
				// denoise
				for(int i = 0; i < size; i++){
					Denoise<true>(height + i);
				}
				for(int i = 0; i < size; i++){
					Denoise<false>(height + i*size);
				}
				
				
				// add randomness
				int count = (int)floorf(dt * 600.f);
				if(count > 400) count = 400;
				for(int i = 0; i < count; i++){
					int ox = rand() % (size - 2);
					int oy = rand() % (size - 2);
					static const float gauss[] = {
						0.225610111284052,
						0.548779777431897,
						0.225610111284052
					};
					float strength = (GetRandom()-GetRandom()) * 0.15f * 100.f;
					for(int x = 0; x < 3; x++)
						for(int y = 0; y < 3; y++){
							velocity[(x+ox)+(y+oy)*size] += strength * gauss[x] * gauss[y];
						}
				}
				
				for(int i = 0; i < samples; i++)
					heightFiltered[i] = height[i];// * height[i] * 100.f;
				
				// build bitmap
				MakeBitmap(heightFiltered);
				
			}
		};
		
#pragma mark - Water Renderer
		
		GLWaterRenderer::GLWaterRenderer(GLRenderer *renderer, client::GameMap *map):
		renderer(renderer),
		device(renderer->GetGLDevice()),
		map(map){
			SPADES_MARK_FUNCTION();
			program = renderer->RegisterProgram("Shaders/Water.program");
			programDepth = renderer->RegisterProgram("Shaders/WaterDepth.program");
			BuildVertices();
			
			tempDepthTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D,
							 tempDepthTexture);
			device->TexImage2D(IGLDevice::Texture2D,
							0,
							IGLDevice::DepthComponent24,
							device->ScreenWidth(),
							device->ScreenHeight(),
							0,
							IGLDevice::DepthComponent,
							IGLDevice::UnsignedInt, NULL);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::ClampToEdge);
			
			tempFramebuffer = device->GenFramebuffer();
			device->BindFramebuffer(IGLDevice::Framebuffer,
								 tempFramebuffer);
			device->FramebufferTexture2D(IGLDevice::Framebuffer,
									  IGLDevice::DepthAttachment,
									  IGLDevice::Texture2D,
									  tempDepthTexture, 0);
			
			// create water color texture
			texture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, texture);
			device->TexImage2D(IGLDevice::Texture2D, 0,
							   IGLDevice::RGBA,
							   map->Width(), map->Height(),
							   0, IGLDevice::RGBA, IGLDevice::UnsignedByte,
							   NULL);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::Repeat);
			
			w = map->Width();
			h = map->Height();
			
			updateBitmapPitch = (w + 31) / 32;
			updateBitmap.resize(updateBitmapPitch * h);
			
			bitmap.resize(w * h);
			std::fill(updateBitmap.begin(), updateBitmap.end(),
					  0xffffffffUL);
			
			// create wave tank simlation
			waveTank = new FFTWaveTank();//new StandardWaveTank(256);
			
			waveTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, waveTexture);
			device->TexImage2D(IGLDevice::Texture2D, 0,
							   IGLDevice::RGBA,
							   waveTank->GetSize(), waveTank->GetSize(),
							   0, IGLDevice::BGRA, IGLDevice::UnsignedByte,
							   NULL);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMagFilter,
								 IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMinFilter,
								 IGLDevice::LinearMipmapLinear);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureWrapS,
								 IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureWrapT,
								 IGLDevice::Repeat);
			
			
		}
		
		struct GLWaterRenderer::Vertex {
			float x, y;
		};
		
		void GLWaterRenderer::BuildVertices() {
			SPADES_MARK_FUNCTION();
			std::vector<Vertex> vertices;
			std::vector<uint16_t> indices;
			
			const int meshSize = 16;
			for(int y = -meshSize; y <= meshSize; y++) {
				for(int x = -meshSize; x <= meshSize; x++){
					Vertex v;
					v.x = (float)(x) / (float)meshSize;
					v.y = (float)(y) / (float)meshSize;
					vertices.push_back(v);
				}
			}
#define VID(x, y) (((x)+meshSize)+((y)+meshSize)*(meshSize * 2 + 1))
			for(int x = -meshSize; x < meshSize; x++){
				for(int y = -meshSize; y < meshSize; y++) {
					indices.push_back(VID(x,y));
					indices.push_back(VID(x+1,y));
					indices.push_back(VID(x,y+1));
					
					indices.push_back(VID(x+1,y));
					indices.push_back(VID(x+1,y+1));
					indices.push_back(VID(x,y+1));
				}
			}
			
			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->BufferData(IGLDevice::ArrayBuffer, sizeof(Vertex)*vertices.size(),
							   vertices.data(), IGLDevice::StaticDraw);
			idxBuffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, idxBuffer);
			device->BufferData(IGLDevice::ArrayBuffer, sizeof(uint16_t)*indices.size(),
							   indices.data(), IGLDevice::StaticDraw);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
			numIndices = indices.size();
		}
		
		GLWaterRenderer::~GLWaterRenderer(){
			SPADES_MARK_FUNCTION();
			device->DeleteBuffer(buffer);
			device->DeleteBuffer(idxBuffer);
			device->DeleteFramebuffer(tempFramebuffer);
			device->DeleteTexture(tempDepthTexture);
			device->DeleteTexture(texture);
			device->DeleteTexture(waveTexture);
			
			waveTank->Join();
			delete waveTank;
		}
		
		void GLWaterRenderer::Render() {
			SPADES_MARK_FUNCTION();
			
			GLProfiler profiler(device, "Render");
			
			GLColorBuffer colorBuffer;
			
			{
				GLProfiler profiler(device, "Preparation");
				colorBuffer = renderer->GetFramebufferManager()->PrepareForWaterRendering(tempFramebuffer);
			}
			
			float fogDist = renderer->GetFogDistance();
			Vector3 fogCol = renderer->GetFogColorForSolidPass();
			fogCol *= fogCol; // linearize
			
			const client::SceneDefinition& def = renderer->GetSceneDef();
			float waterLevel = 63.f;
			float waterDist = def.viewOrigin.z - waterLevel;
			float waterRange = sqrtf(std::max(0.f, fogDist*fogDist-waterDist*waterDist));
			
			Matrix4 mat = Matrix4::Translate(def.viewOrigin.x,
											 def.viewOrigin.y,
											 waterLevel);
			mat = mat * Matrix4::Scale(waterRange, waterRange, 1.f);
			
			
			GLProfiler profiler2(device, "Draw Plane");
			
			// do color
			device->DepthFunc(IGLDevice::Less);
			device->ColorMask(true, true, true, true);
			{
				GLProgram *prg = program;
				prg->Use();
				
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				static GLProgramUniform modelMatrix("modelMatrix");
				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				static GLProgramUniform fogDistance("fogDistance");
				static GLProgramUniform fogColor("fogColor");
				static GLProgramUniform zNearFar("zNearFar");
				static GLProgramUniform viewOrigin("viewOrigin");
				static GLProgramUniform displaceScale("displaceScale");
				static GLProgramUniform fovTan("fovTan");
				static GLProgramUniform waterPlane("waterPlane");
				
				projectionViewModelMatrix(prg);
				modelMatrix(prg);
				viewModelMatrix(prg);
				fogDistance(prg);
				fogColor(prg);
				zNearFar(prg);
				viewOrigin(prg);
				displaceScale(prg);
				fovTan(prg);
				waterPlane(prg);
				
				projectionViewModelMatrix.SetValue(renderer->GetProjectionViewMatrix() * mat);
				modelMatrix.SetValue(mat);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * mat);
				fogDistance.SetValue(fogDist);
				fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);
				zNearFar.SetValue(def.zNear, def.zFar);
				viewOrigin.SetValue(def.viewOrigin.x,
									def.viewOrigin.y,
									def.viewOrigin.z);
				displaceScale.SetValue(1.f / renderer->ScreenWidth() / tanf(def.fovX * .5f),
									   1.f / renderer->ScreenHeight() / tanf(def.fovY) * .5f);
				fovTan.SetValue(tanf(def.fovX * .5f), -tanf(def.fovY * .5f),
								-tanf(def.fovX * .5f), tanf(def.fovY * .5f));
				
				// make water plane in view coord
				Matrix4 wmat = renderer->GetViewMatrix() * mat;
				Vector3 dir = wmat.GetAxis(2);
				waterPlane.SetValue(dir.x, dir.y, dir.z,
									-Vector3::Dot(dir, wmat.GetOrigin()));
				
				static GLProgramUniform screenTexture("screenTexture");
				static GLProgramUniform depthTexture("depthTexture");
				static GLProgramUniform textureUnif("texture");
				static GLProgramUniform waveTextureUnif("waveTexture");
				
				screenTexture(prg);
				depthTexture(prg);
				textureUnif(prg);
				waveTextureUnif(prg);
				
				device->ActiveTexture(0);
				device->BindTexture(IGLDevice::Texture2D, colorBuffer.GetTexture());
				screenTexture.SetValue(0);
				// depth is not interpolated, so color shouldn't be
				device->TexParamater(IGLDevice::Texture2D,
									 IGLDevice::TextureMagFilter,
									 IGLDevice::Nearest);
				device->TexParamater(IGLDevice::Texture2D,
									 IGLDevice::TextureMinFilter,
									 IGLDevice::Nearest);
				
				device->ActiveTexture(1);
				device->BindTexture(IGLDevice::Texture2D, tempDepthTexture);
				depthTexture.SetValue(1);
				
				device->ActiveTexture(2);
				device->BindTexture(IGLDevice::Texture2D, texture);
				textureUnif.SetValue(2);
				
				device->ActiveTexture(3);
				device->BindTexture(IGLDevice::Texture2D, waveTexture);
				waveTextureUnif.SetValue(3);
				
				static GLShadowShader shadowShader;
				shadowShader(renderer, prg, 4);
				
				static GLProgramAttribute positionAttribute("positionAttribute");
				
				positionAttribute(prg);
				
				device->EnableVertexAttribArray(positionAttribute(), true);
				
				device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
				device->VertexAttribPointer(positionAttribute(),
											2, IGLDevice::FloatType,
											false, sizeof(Vertex), NULL);
				device->BindBuffer(IGLDevice::ArrayBuffer, 0);
				
				device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);
				device->DrawElements(IGLDevice::Triangles, numIndices,
									 IGLDevice::UnsignedShort, NULL);
				
				device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
				
				device->EnableVertexAttribArray(positionAttribute(), false);
				
				
				device->ActiveTexture(0);
				// restore filter mode for color buffer
				device->TexParamater(IGLDevice::Texture2D,
									 IGLDevice::TextureMagFilter,
									 IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D,
									 IGLDevice::TextureMinFilter,
									 IGLDevice::Linear);
			}
			
		}
		
		static uint32_t LinearlizeColor(uint32_t v){
			int r = (uint8_t)(v);
			int g = (uint8_t)(v >> 8);
			int b = (uint8_t)(v >> 16);
			r = (r * r + 128) >> 8;
			g = (g * g + 128) >> 8;
			b = (b * b + 128) >> 8;
			return r|(g<<8)|(b<<16);
		}
		
		void GLWaterRenderer::Update(float dt) {
			SPADES_MARK_FUNCTION();
			GLProfiler profiler(device, "Update");
			
			// update wavetank simulation
			{
				GLProfiler profiler(device, "Waiting for Simulation To Done");
				waveTank->Join();
			}
			{
				{
					GLProfiler profiler(device, "Upload");
					device->BindTexture(IGLDevice::Texture2D, waveTexture);
					device->TexSubImage2D(IGLDevice::Texture2D, 0,
										  0, 0, waveTank->GetSize(), waveTank->GetSize(),
										  IGLDevice::BGRA, IGLDevice::UnsignedByte,
										  waveTank->GetBitmap());
				}
				{
					GLProfiler profiler(device, "Generate Mipmap");
					device->GenerateMipmap(IGLDevice::Texture2D);
				}
			}
			waveTank->SetTimeStep(dt);
			waveTank->Start();
			
			{
				GLProfiler profiler(device, "Upload Water Color Texture");
				device->BindTexture(IGLDevice::Texture2D, texture);
				for(size_t i = 0; i < updateBitmap.size(); i++){
					int y = i / updateBitmapPitch;
					int x = (i - y * updateBitmapPitch) * 32;
					if(updateBitmap[i] == 0)
						continue;
					
					
					uint32_t pixels[32];
					for(int j = 0; j < 32; j++){
						uint32_t col = map->GetColor(x+j, y, 63);
						
						col = LinearlizeColor(col);
						
						pixels[j] = col;
						//pixels[j] = GeneratePixel(x + j, y);
					}
					
					device->TexSubImage2D(IGLDevice::Texture2D,
										  0, x, y, 32, 1,
										  IGLDevice::RGBA, IGLDevice::UnsignedByte,
										  pixels);
					
					updateBitmap[i] = 0;
				}
			}
		}
		
		void GLWaterRenderer::MarkUpdate(int x, int y) {
			x &= w - 1;
			y &= h - 1;
			updateBitmap[(x >> 5) + y * updateBitmapPitch] |=
			1UL << (x & 31);
		}
		
		void GLWaterRenderer::GameMapChanged(int x, int y, int z,
											 client::GameMap *map) {
			if(map != this->map)
				return;
			if(z < 63)
				return;
			MarkUpdate(x, y);
		}
	}
}
