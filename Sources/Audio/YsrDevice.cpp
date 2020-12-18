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

#include <Imports/SDL.h>

#include "YsrDevice.h"
#include <Client/GameMap.h>
#include <Client/IAudioChunk.h>
#include <Core/DynamicLibrary.h>
#include <Core/FileManager.h>
#include <Core/IAudioStream.h>
#include <Core/IStream.h>
#include <Core/Settings.h>
#include <Core/AudioStream.h>

#if defined(__APPLE__)
DEFINE_SPADES_SETTING(s_ysrDriver, "libysrspades.dylib");
#elif defined(WIN32)
DEFINE_SPADES_SETTING(s_ysrDriver, "YSRSpades.dll");
#else
DEFINE_SPADES_SETTING(s_ysrDriver, "libysrspades.so");
#endif

SPADES_SETTING(s_volume);
extern int s_volume_previous;
extern float dBPrevious;

DEFINE_SPADES_SETTING(s_ysrNumThreads, "2");
SPADES_SETTING(s_maxPolyphonics);
SPADES_SETTING(s_gain);
DEFINE_SPADES_SETTING(s_ysrBufferSize, "1024");
DEFINE_SPADES_SETTING(s_ysrDebug, "0");

namespace spades {
	namespace audio {

		static spades::DynamicLibrary *library = nullptr;

		class YsrContext {
			struct ContextPrivate;
			ContextPrivate *priv;

		public:
			struct Buffer;
			struct SpatializeResult;
			struct YVec3 {
				float x, y, z;
				YVec3() : x(0), y(0), z(0) { ; }
				YVec3(const Vector3 &v) : x(v.x), y(v.y), z(v.z) {}
				operator Vector3() const { return MakeVector3(x, y, z); }
			};
			struct InitParam {
				double samplingRate;
				void (*spatializerCallback)(const YVec3 *origin, SpatializeResult *,
				                            void *userData);
				void *spatializerUserData;
				int numThreads;
				int maxPolyphonics;
				void (*debugCallback)(const char *, void *);
				void *debugUserData;
			};
			struct ReverbParam {
				float reflections;
				float roomVolume;
				float roomArea;
				float roomSize;
				float feedbackness;
			};
			struct SpatializeResult {
				float directGain;
			};
			struct BufferParam {
				const void *data;
				int sampleBits;
				int numChannels;
				int numSamples;
				double samplingRate;
			};
			struct PlayParam {
				float volume;
				float pitch;
				float referenceDistance;
			};

		private:
			void (*init)(ContextPrivate *, const InitParam *);
			void (*destroy)(ContextPrivate *);
			const char *(*checkError)(ContextPrivate *);
			Buffer *(*createBuffer)(ContextPrivate *, const BufferParam *);
			void (*freeBuffer)(ContextPrivate *, Buffer *);
			void (*respatialize)(ContextPrivate *, const YVec3 *eye, const YVec3 *front,
			                     const YVec3 *up, const ReverbParam *);
			void (*playAbsolute)(ContextPrivate *, Buffer *, const YVec3 *, const PlayParam *);
			void (*playRelative)(ContextPrivate *, Buffer *, const YVec3 *, const PlayParam *);
			void (*playLocal)(ContextPrivate *, Buffer *, const PlayParam *);
			void (*render)(ContextPrivate *, float *, int);

		public:
			void Init(const InitParam &param) { init(priv, &param); }

			void Destroy() { destroy(priv); }

			Buffer *CreateBuffer(const BufferParam &param) { return createBuffer(priv, &param); }

			void FreeBuffer(Buffer *b) { freeBuffer(priv, b); }

			void Respatialize(const Vector3 &eye, const Vector3 &front, const Vector3 &up,
			                  const ReverbParam &param) {
				YVec3 _eye(eye), _front(front), _up(up);
				respatialize(priv, &_eye, &_front, &_up, &param);
			}

			void CheckError() {
				const char *err = checkError(priv);
				if (err) {
					SPRaise("YSR Error: %s", err);
				}
			}

			void PlayLocal(Buffer *buffer, const PlayParam &param) {
				playLocal(priv, buffer, &param);
			}

			void PlayRelative(Buffer *buffer, const Vector3 &v, const PlayParam &param) {
				YVec3 origin(v);
				playRelative(priv, buffer, &origin, &param);
			}

			void PlayAbsolute(Buffer *buffer, const Vector3 &v, const PlayParam &param) {
				YVec3 origin(v);
				playAbsolute(priv, buffer, &origin, &param);
			}

			void Render(float *buffer, int numSamples) { render(priv, buffer, numSamples); }
		};

		class YsrBuffer;

		class YsrDriver {
		public:
		private:
			YsrContext ctx;

		public:
			YsrDriver();
			~YsrDriver();

			void Init(const YsrContext::InitParam &param);
			std::shared_ptr<YsrBuffer> CreateBuffer(const YsrContext::BufferParam &param);
			void Respatialize(const Vector3 &eye, const Vector3 &front, const Vector3 &up,
			                  const YsrContext::ReverbParam &);
			void PlayLocal(const std::shared_ptr<YsrBuffer> &buffer,
			               const YsrContext::PlayParam &param);
			void PlayAbsolute(const std::shared_ptr<YsrBuffer> &buffer, const Vector3 &origin,
			                  const YsrContext::PlayParam &param);
			void PlayRelative(const std::shared_ptr<YsrBuffer> &buffer, const Vector3 &origin,
			                  const YsrContext::PlayParam &param);
			void Render(float *buffer, int numSamples);
		};

		class YsrBuffer {
			YsrContext &context;
			YsrContext::Buffer *buffer;

		public:
			YsrBuffer(YsrContext &context, YsrContext::Buffer *buffer);
			~YsrBuffer();
			YsrContext::Buffer *GetBuffer() { return buffer; }
		};

		YsrDriver::YsrDriver() {
			if (!library) {
				library = new spades::DynamicLibrary(s_ysrDriver.CString());
				SPLog("'%s' loaded", s_ysrDriver.CString());
			}

			typedef const char *(*InitializeFunc)(const char *magic, uint32_t version,
			                                      YsrContext *);
			auto initFunc = reinterpret_cast<InitializeFunc>(library->GetSymbol("YsrInitialize"));
			const char *a = (*initFunc)("OpenYsrSpades", 1, &ctx);
			if (a) {
				SPRaise("Failed to initialize YSR interface: %s", a);
			}
		}

		bool YsrDevice::TryLoadYsr() {
			try {
				if (!library) {
					library = new spades::DynamicLibrary(s_ysrDriver.CString());
					SPLog("'%s' loaded", s_ysrDriver.CString());
				}
				return true;
			} catch (...) {
				return false;
			}
		}

		YsrDriver::~YsrDriver() { ctx.Destroy(); }

		void YsrDriver::Init(const YsrContext::InitParam &param) {
			ctx.Init(param);
			ctx.CheckError();
		}

		void YsrDriver::Render(float *buffer, int numSamples) {
			ctx.Render(buffer, numSamples);
			ctx.CheckError();
		}

		std::shared_ptr<YsrBuffer> YsrDriver::CreateBuffer(const YsrContext::BufferParam &param) {
			auto *buf = ctx.CreateBuffer(param);
			try {
				ctx.CheckError();
			} catch (...) {
				if (buf != nullptr) {
					ctx.FreeBuffer(buf);
				}
				throw;
			}
			return std::make_shared<YsrBuffer>(ctx, buf);
		}

		void YsrDriver::Respatialize(const spades::Vector3 &eye, const spades::Vector3 &front,
		                             const spades::Vector3 &up,
		                             const YsrContext::ReverbParam &param) {
			ctx.Respatialize(eye, front, up, param);
			ctx.CheckError();
		}

		void YsrDriver::PlayLocal(const std::shared_ptr<YsrBuffer> &buffer,
		                          const YsrContext::PlayParam &param) {
			ctx.PlayLocal(buffer->GetBuffer(), param);
			ctx.CheckError();
		}

		void YsrDriver::PlayAbsolute(const std::shared_ptr<YsrBuffer> &buffer,
		                             const Vector3 &origin, const YsrContext::PlayParam &param) {
			ctx.PlayAbsolute(buffer->GetBuffer(), origin, param);
			ctx.CheckError();
		}

		void YsrDriver::PlayRelative(const std::shared_ptr<YsrBuffer> &buffer,
		                             const Vector3 &origin, const YsrContext::PlayParam &param) {
			ctx.PlayRelative(buffer->GetBuffer(), origin, param);
			ctx.CheckError();
		}

		YsrBuffer::YsrBuffer(YsrContext &context, YsrContext::Buffer *buffer)
		    : context(context), buffer(buffer) {}

		YsrBuffer::~YsrBuffer() { context.FreeBuffer(buffer); }

		class YsrAudioChunk : public client::IAudioChunk {
			std::shared_ptr<YsrBuffer> buffer;

		protected:
			~YsrAudioChunk() {}

		public:
			YsrAudioChunk(std::shared_ptr<YsrDriver> driver, IAudioStream *stream) {
				std::vector<char> buffer;
				size_t bytesPerSample = stream->GetNumChannels();
				if (bytesPerSample < 1 || bytesPerSample > 2) {
					SPRaise("Unsupported channel count");
				}
				switch (stream->GetSampleFormat()) {
					case IAudioStream::SignedShort: bytesPerSample *= 2; break;
					case IAudioStream::UnsignedByte: bytesPerSample *= 1; break;
					case IAudioStream::SingleFloat: bytesPerSample *= 4; break;
					default: SPRaise("Unsupported audio format");
				}
				if (stream->GetNumSamples() > 128 * 1024 * 1024) {
					SPRaise("Audio data too long");
				}
				buffer.resize(static_cast<size_t>(stream->GetNumSamples()) * bytesPerSample);
				stream->Read(reinterpret_cast<void *>(buffer.data()), buffer.size());

				YsrContext::BufferParam param;
				param.data = reinterpret_cast<void *>(buffer.data());
				param.numChannels = stream->GetNumChannels();
				param.numSamples = static_cast<int>(stream->GetNumSamples());
				param.samplingRate = stream->GetSamplingFrequency();
				switch (stream->GetSampleFormat()) {
					case IAudioStream::SignedShort: param.sampleBits = 16; break;
					case IAudioStream::UnsignedByte: param.sampleBits = 8; break;
					case IAudioStream::SingleFloat: param.sampleBits = 32; break;
				}

				this->buffer = driver->CreateBuffer(param);
			}
			std::shared_ptr<YsrBuffer> GetBuffer() { return buffer; }
		};

		struct SdlAudioDevice {
			SDL_AudioDeviceID id;
			SDL_AudioSpec spec;

			SdlAudioDevice(const char *deviceId, int isCapture, const SDL_AudioSpec &spec,
			               int allowedChanges)
			    : id(0) {
				SDL_InitSubSystem(SDL_INIT_AUDIO);
				id = SDL_OpenAudioDevice(deviceId, isCapture, &spec, &this->spec, allowedChanges);
				if (id == 0) {
					SPRaise("Failed to initialize the audio device: %s", SDL_GetError());
				}
			}

			~SdlAudioDevice() {
				if (id != 0)
					SDL_CloseAudioDevice(id);
			}

			SDL_AudioDeviceID operator()() const { return id; }
		};

		static void DebugLog(const char *msg, void *) { SPLog("YSR Debug: %s", msg); }

		YsrDevice::YsrDevice()
		    : driver(new YsrDriver()),
		      gameMap(nullptr),
		      listenerPosition(MakeVector3(0, 0, 0)),
		      roomHistoryPos(0) {
			SDL_AudioSpec spec;
			spec.callback = reinterpret_cast<SDL_AudioCallback>(RenderCallback);
			spec.userdata = this;
			spec.format = AUDIO_F32SYS;
			spec.freq = 44100;
			spec.samples = (int)s_ysrBufferSize;
			spec.channels = 2;

			sdlAudioDevice = std::unique_ptr<SdlAudioDevice>(
			  new SdlAudioDevice(nullptr, SDL_FALSE, spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE));

			YsrContext::InitParam param;
			param.maxPolyphonics = s_maxPolyphonics;
			param.numThreads = s_ysrNumThreads;
			param.samplingRate = static_cast<double>(sdlAudioDevice->spec.freq);
			param.spatializerCallback =
			  reinterpret_cast<void (*)(const YsrContext::YVec3 *, YsrContext::SpatializeResult *,
			                            void *)>(SpatializeCallback);
			param.spatializerUserData = this;
			if (s_ysrDebug) {
				param.debugCallback = DebugLog;
			} else {
				param.debugCallback = nullptr;
			}
			param.debugUserData = nullptr;

			driver->Init(param);

			std::fill(roomHistory.begin(), roomHistory.end(), 20000.f);
			std::fill(roomFeedbackHistory.begin(), roomFeedbackHistory.end(), 0.f);

			SDL_PauseAudioDevice((*sdlAudioDevice)(), 0);
		}

		YsrDevice::~YsrDevice() {
			for (auto chunk = chunks.begin(); chunk != chunks.end(); ++chunk) {
				chunk->second->Release();
			}
			if (this->gameMap)
				this->gameMap->Release();
		}

		void YsrDevice::RenderCallback(YsrDevice *self, float *stream, int numBytes) {
			self->Render(stream, numBytes);
		}

		void YsrDevice::SpatializeCallback(const void *yorigin, void *_result, YsrDevice *self) {
			self->Spatialize(yorigin, _result);
		}

		void YsrDevice::Render(float *stream, int numBytes) {
			driver->Render(stream, numBytes / 8);
		}

		void YsrDevice::Spatialize(const void *yorigin, void *_result) {
			Vector3 origin(*reinterpret_cast<const YsrContext::YVec3 *>(yorigin));
			auto &result = *reinterpret_cast<YsrContext::SpatializeResult *>(_result);

			// check obstruction
			if (gameMap) {
				Vector3 eye = listenerPosition;
				Vector3 pos = origin;
				Vector3 checkPos;
				result.directGain = 0.4f;
				for (int x = -1; x <= 1; x++)
					for (int y = -1; y <= 1; y++)
						for (int z = -1; z <= 1; z++) {
							IntVector3 hitPos;
							checkPos.x = pos.x + (float)x * .2f;
							checkPos.y = pos.y + (float)y * .2f;
							checkPos.z = pos.z + (float)z * .2f;
							if (!gameMap->CastRay(eye, (checkPos - eye).Normalize(),
							                      (checkPos - eye).GetLength(), hitPos)) {
								result.directGain = 1.f;
							}
						}
			} else {
				result.directGain = 1.f;
			}
		}

		auto YsrDevice::CreateChunk(const char *name) -> YsrAudioChunk * {
			SPADES_MARK_FUNCTION();

			IAudioStream *as = OpenAudioStream(name);

			try {
				YsrAudioChunk *ch = new YsrAudioChunk(driver, as);
				delete as;
				return ch;
			} catch (...) {
				delete as;
				throw;
			}
		}

		client::IAudioChunk *YsrDevice::RegisterSound(const char *name) {
			SPADES_MARK_FUNCTION();

			auto it = chunks.find(name);
			if (it == chunks.end()) {
				auto *c = CreateChunk(name);
				chunks[name] = c;
				c->AddRef();
				return c;
			}
			it->second->AddRef();
			return it->second;
		}

		void YsrDevice::ClearCache() {
			SPADES_MARK_FUNCTION();

			for (auto &chunk: chunks) {
				chunk.second->Release();
			}
			chunks.clear();
		}

		void YsrDevice::SetGameMap(client::GameMap *gameMap) {
			SPADES_MARK_FUNCTION();
			auto *old = this->gameMap;
			this->gameMap = gameMap;
			if (this->gameMap)
				this->gameMap->AddRef();
			if (old)
				old->Release();
		}

		void YsrDevice::Respatialize(const spades::Vector3 &eye, const spades::Vector3 &front,
		                             const spades::Vector3 &up) {
			SPADES_MARK_FUNCTION();

			listenerPosition = eye;

			YsrContext::ReverbParam reverbParam;
			float maxDistance = 40.f;
			float reflections;
			float roomVolume;
			float roomArea;
			float roomSize;
			float feedbackness = 0.f;
			auto *map = gameMap;
			if (map == NULL) {
				reflections = 0.f;
				roomVolume = 1.f;
				roomArea = 1.f;
				roomSize = 10.f;
			} else {
				// do raycast
				Vector3 rayFrom = eye;
				Vector3 rayTo;

				for (int rays = 0; rays < 4; rays++) {
					rayTo.x = SampleRandomFloat() - SampleRandomFloat();
					rayTo.y = SampleRandomFloat() - SampleRandomFloat();
					rayTo.z = SampleRandomFloat() - SampleRandomFloat();
					rayTo = rayTo.Normalize();

					IntVector3 hitPos;
					bool hit = map->CastRay(rayFrom, rayTo, maxDistance, hitPos);
					if (hit) {
						Vector3 hitPosf = {(float)hitPos.x, (float)hitPos.y, (float)hitPos.z};
						roomHistory[roomHistoryPos] = (hitPosf - rayFrom).GetLength();
					} else {
						roomHistory[roomHistoryPos] = maxDistance * 2.f;
					}

					if (hit) {
						bool hit2 = map->CastRay(rayFrom, -rayTo, maxDistance, hitPos);
						if (hit2)
							roomFeedbackHistory[roomHistoryPos] = 1.f;
						else
							roomFeedbackHistory[roomHistoryPos] = 0.f;
					}

					roomHistoryPos++;
					if (roomHistoryPos == (int)roomHistory.size())
						roomHistoryPos = 0;
				}

				// monte-carlo integration
				unsigned int rayHitCount = 0;
				roomVolume = 0.f;
				roomArea = 0.f;
				roomSize = 0.f;
				feedbackness = 0.f;
				for (size_t i = 0; i < roomHistory.size(); i++) {
					float dist = roomHistory[i];
					if (dist < maxDistance) {
						rayHitCount++;
						roomVolume += dist * dist;
						roomArea += dist;
						roomSize += dist;
					}

					feedbackness += roomFeedbackHistory[i];
				}

				if (rayHitCount > roomHistory.size() / 4) {
					roomVolume /= (float)rayHitCount;
					roomVolume *= 4.f / 3.f * static_cast<float>(M_PI);
					roomArea /= (float)rayHitCount;
					roomArea *= 4.f * static_cast<float>(M_PI);
					roomSize /= (float)rayHitCount;
					reflections = (float)rayHitCount / (float)roomHistory.size();
				} else {
					roomVolume = 8.f;
					reflections = 0.1f;
					roomArea = 100.f;
					roomSize = 100.f;
				}

				feedbackness /= (float)roomHistory.size();
			}

			reverbParam.feedbackness = feedbackness;
			reverbParam.reflections = reflections;
			reverbParam.roomArea = roomArea;
			reverbParam.roomSize = roomSize;
			reverbParam.roomVolume = roomVolume;

			driver->Respatialize(eye, front, up, reverbParam);
		}

		static YsrContext::PlayParam TranslateParam(const client::AudioParam &base) {
			YsrContext::PlayParam param;
			param.pitch = base.pitch;
			param.referenceDistance = base.referenceDistance;

			// Update master volume control
			if (s_volume_previous != (int)s_volume) {
				// update the previous volume
				s_volume_previous = (int)s_volume;
				// compute the new dB level, where 27.71373379 ~ 10^(1/log(2)), and update the
				// master gain to it
				if ((int)s_volume == 0) {
					dBPrevious = 0;
				} else {
					dBPrevious = powf(27.71373379f, log(((float)s_volume) / 100.0f));
				}
			}
			param.volume = dBPrevious;
			
			return param;
		}

		void YsrDevice::Play(client::IAudioChunk *c, const Vector3 &origin,
		                     const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if (chunk == nullptr)
				SPRaise("Invalid chunk: null or invalid type.");

			driver->PlayAbsolute(chunk->GetBuffer(), origin, TranslateParam(param));
		}
		void YsrDevice::PlayLocal(client::IAudioChunk *c, const Vector3 &origin,
		                          const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if (chunk == nullptr)
				SPRaise("Invalid chunk: null or invalid type.");

			driver->PlayRelative(chunk->GetBuffer(), origin, TranslateParam(param));
		}
		void YsrDevice::PlayLocal(client::IAudioChunk *c, const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if (chunk == nullptr)
				SPRaise("Invalid chunk: null or invalid type.");

			driver->PlayLocal(chunk->GetBuffer(), TranslateParam(param));
		}
	}
}
