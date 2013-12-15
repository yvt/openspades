//
//  YsrDevice.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 12/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "YsrDevice.h"
#include <Client/IAudioChunk.h>
#include <Client/GameMap.h>
#include <Core/Settings.h>
#include <Core/DynamicLibrary.h>
#include <Core/FileManager.h>
#include <Core/IAudioStream.h>
#include <Core/WavAudioStream.h>
#include <cstdlib>

#if defined(__APPLE__)
SPADES_SETTING(s_ysrDriver, "libysrspades.dylib");
#elif defined(WIN32)
SPADES_SETTING(s_ysrDriver, "YSR for OpenSpades.dll");
#else
SPADES_SETTING(s_ysrDriver, "libysrspades.so");
#endif

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
				YVec3() = default;
				YVec3(const Vector3& v):
				x(v.x), y(v.y), z(v.z) {}
			};
			struct InitParam {
				double samplingRate;
				void (*spatializerCallback)(const YVec3 *origin,
											SpatializeResult *,
											void *userData);
				void *spatializerUserData;
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
			Buffer *(*createBuffer)(ContextPrivate *,
									const BufferParam *);
			void (*freeBuffer)(ContextPrivate *,
							   Buffer *);
			void (*respatialize)(ContextPrivate *,
								 const YVec3 *eye,
								 const YVec3 *front,
								 const YVec3 *up,
								 const ReverbParam *);
			void (*playAbsolute)(ContextPrivate *, Buffer *, const YVec3 *, const PlayParam *);
			void (*playRelative)(ContextPrivate *, Buffer *, const YVec3 *, const PlayParam *);
			void (*playLocal)(ContextPrivate *, Buffer *, const PlayParam *);
			
		public:
			void Init(const InitParam& param) {
				init(priv, &param);
			}
			
			void Destroy() {
				destroy(priv);
			}
			
			Buffer *CreateBuffer(const BufferParam& param) {
				return createBuffer(priv, &param);
			}
			
			void FreeBuffer(Buffer *b) {
				freeBuffer(priv, b);
			}
			
			void Respatialize(const Vector3& eye,
							  const Vector3& front,
							  const Vector3& up,
							  const ReverbParam& param) {
				YVec3 _eye(eye), _front(front), _up(up);
				respatialize(priv, &_eye, &_front, &_up, &param);
			}
		
			void CheckError() {
				const char *err = checkError(priv);
				if(err) {
					SPRaise("YSR Error: %s", err);
				}
			}
			
			void PlayLocal(Buffer *buffer, const PlayParam& param) {
				playLocal(priv, buffer, &param);
			}
			
			void PlayRelative(Buffer *buffer, const Vector3& v, const PlayParam& param) {
				YVec3 origin(v);
				playRelative(priv, buffer, &origin, &param);
			}
			
			void PlayAbsolute(Buffer *buffer, const Vector3& v, const PlayParam& param) {
				YVec3 origin(v);
				playAbsolute(priv, buffer, &origin, &param);
			}
		};
		
		class YsrBuffer;
		
		class YsrDriver {
		public:
		private:
			YsrContext ctx;
		public:
			YsrDriver();
			~YsrDriver() ;
			
			void Init(const YsrContext::InitParam& param);
			std::shared_ptr<YsrBuffer> CreateBuffer(const YsrContext::BufferParam& param);
			void Respatialize(const Vector3& eye,
							  const Vector3& front,
							  const Vector3& up,
							  const YsrContext::ReverbParam&);
			void PlayLocal(const std::shared_ptr<YsrBuffer>& buffer,
						   const YsrContext::PlayParam& param);
			void PlayAbsolute(const std::shared_ptr<YsrBuffer>& buffer,
						   const Vector3& origin,
						   const YsrContext::PlayParam& param);
			void PlayRelative(const std::shared_ptr<YsrBuffer>& buffer,
						   const Vector3& origin,
						   const YsrContext::PlayParam& param);
		};
		
		class YsrBuffer {
			YsrContext& context;
			YsrContext::Buffer *buffer;
		public:
			YsrBuffer(YsrContext& context,
					  YsrContext::Buffer *buffer);
			~YsrBuffer();
			YsrContext::Buffer *GetBuffer() {
				return buffer;
			}
		};
		
		YsrDriver::YsrDriver() {
			if(!library){
				library = new spades::DynamicLibrary(s_ysrDriver.CString());
				SPLog("'%s' loaded", s_ysrDriver.CString());
			}
			
			using InitializeFunc = const YsrContext *(*)(const char *magic, uint32_t version);
			auto initFunc = reinterpret_cast<InitializeFunc>(library->GetSymbol("YsrInitialize"));
			const auto *a = (*initFunc)("OpenYsrSpades", 1);
			if(a == nullptr) {
				SPRaise("Failed to initialize YSR interface for OpenSpades.");
			}
			ctx = *a;
		}
		
		YsrDriver::~YsrDriver() {
			ctx.Destroy();
		}
		
		void YsrDriver::Init(const YsrContext::InitParam &param) {
			ctx.Init(param);
			ctx.CheckError();
		}
		
		std::shared_ptr<YsrBuffer> YsrDriver::CreateBuffer(const YsrContext::BufferParam &param) {
			auto* buf = ctx.CreateBuffer(param);
			try{
				ctx.CheckError();
			}catch(...){
				if(buf != nullptr) {
					ctx.FreeBuffer(buf);
				}
				throw;
			}
			return std::make_shared<YsrBuffer>(ctx, buf);
		}
		
		void YsrDriver::Respatialize(const spades::Vector3 &eye,
									 const spades::Vector3 &front,
									 const spades::Vector3 &up,
									 const YsrContext::ReverbParam& param) {
			ctx.Respatialize(eye, front, up, param);
			ctx.CheckError();
		}
		
		void YsrDriver::PlayLocal(const std::shared_ptr<YsrBuffer>& buffer,
					   const YsrContext::PlayParam& param) {
			ctx.PlayLocal(buffer->GetBuffer(), param);
			ctx.CheckError();
		}
		
		void YsrDriver::PlayAbsolute(const std::shared_ptr<YsrBuffer>& buffer,
									 const Vector3& origin,
								  const YsrContext::PlayParam& param) {
			ctx.PlayAbsolute(buffer->GetBuffer(), origin, param);
			ctx.CheckError();
		}
		
		void YsrDriver::PlayRelative(const std::shared_ptr<YsrBuffer>& buffer,
									 const Vector3& origin,
									 const YsrContext::PlayParam& param) {
			ctx.PlayRelative(buffer->GetBuffer(), origin, param);
			ctx.CheckError();
		}
		
		YsrBuffer::YsrBuffer(YsrContext& context,
							 YsrContext::Buffer *buffer):
		context(context),
		buffer(buffer){
		}
		
		YsrBuffer::~YsrBuffer() {
			context.FreeBuffer(buffer);
		}
		
		class YsrAudioChunk: public client::IAudioChunk {
			std::shared_ptr<YsrBuffer> buffer;
			
		protected:
			virtual ~YsrAudioChunk() {
				
			}
		public:
			YsrAudioChunk(std::shared_ptr<YsrDriver> driver,
						  IAudioStream *stream) {
				std::vector<char> buffer;
				size_t bytesPerSample = stream->GetNumChannels();
				if(bytesPerSample < 1 || bytesPerSample > 2) {
					SPRaise("Unsupported channel count");
				}
				switch(stream->GetSampleFormat()) {
					case IAudioStream::SignedShort:
						bytesPerSample *= 2;
						break;
					case IAudioStream::UnsignedByte:
						bytesPerSample *= 1;
						break;
					case IAudioStream::SingleFloat:
						bytesPerSample *= 4;
						break;
					default:
						SPRaise("Unsupported audio format");
				}
				if(stream->GetNumSamples() > 128 * 1024 * 1024) {
					SPRaise("Audio data too long");
				}
				buffer.resize(static_cast<size_t>(stream->GetNumSamples()) * bytesPerSample);
				stream->Read(reinterpret_cast<void*>(buffer.data()), buffer.size());
				
				YsrContext::BufferParam param;
				param.data = reinterpret_cast<void *>(buffer.data());
				param.numChannels = stream->GetNumChannels();
				param.numSamples = static_cast<int>(stream->GetNumSamples());
				switch(stream->GetSampleFormat()) {
					case IAudioStream::SignedShort:
						param.sampleBits = 16;
						break;
					case IAudioStream::UnsignedByte:
						param.sampleBits = 8;
						break;
					case IAudioStream::SingleFloat:
						param.sampleBits = 32;
						break;
				}
				
				this->buffer = driver->CreateBuffer(param);
			}
			std::shared_ptr<YsrBuffer> GetBuffer() {
				return buffer;
			}
			
		};
		
		YsrDevice::YsrDevice():
		gameMap(nullptr),
		driver(new YsrDriver()),
		roomHistoryPos(0)
		{
			
		}
		
		YsrDevice::~YsrDevice() {
			for(auto& chunk: chunks){
				chunk.second->Release();
			}
			if(this->gameMap)
				this->gameMap->Release();
		}
		
		auto YsrDevice::CreateChunk(const char *name) -> YsrAudioChunk * {
			SPADES_MARK_FUNCTION();
			
			std::unique_ptr<IStream> stream(FileManager::OpenForReading(name));
			std::unique_ptr<IAudioStream> as(new WavAudioStream(stream.get(), true));
			
			return new YsrAudioChunk(driver, as.get());
		}
		
		client::IAudioChunk *YsrDevice::RegisterSound(const char *name) {
			SPADES_MARK_FUNCTION();
			
			auto it = chunks.find(name);
			if(it == chunks.end()){
				auto *c = CreateChunk(name);
				chunks[name] = c;
				it->second->AddRef();
				return c;
			}
			it->second->AddRef();
			return it->second;
		}
		
		void YsrDevice::SetGameMap(client::GameMap *gameMap) {
			SPADES_MARK_FUNCTION();
			auto *old = this->gameMap;
			this->gameMap = gameMap;
			if(this->gameMap) this->gameMap->AddRef();
			if(old) old->Release();
		}
		
		static float NextRandom() {
			return (float)std::rand() /(float)RAND_MAX;
		}
		
		void YsrDevice::Respatialize(const spades::Vector3 &eye,
									 const spades::Vector3 &front,
									 const spades::Vector3 &up) {
			SPADES_MARK_FUNCTION();
			
			YsrContext::ReverbParam reverbParam;
			float maxDistance = 40.f;
			float reflections;
			float roomVolume;
			float roomArea;
			float roomSize;
			float feedbackness = 0.f;
			auto *map = gameMap;
			if(map == NULL){
				reflections = 0.f;
				roomVolume = 1.f;
				roomArea = 1.f;
				roomSize = 10.f;
			}else{
				// do raycast
				Vector3 rayFrom = eye;
				Vector3 rayTo;
				
				for(int rays = 0; rays < 4; rays++){
					rayTo.x = NextRandom() - NextRandom();
					rayTo.y = NextRandom() - NextRandom();
					rayTo.z = NextRandom() - NextRandom();
					rayTo = rayTo.Normalize();
					
					IntVector3 hitPos;
					bool hit = map->CastRay(rayFrom, rayTo, maxDistance, hitPos);
					if(hit){
						Vector3 hitPosf = {(float)hitPos.x, (float)hitPos.y, (float)hitPos.z};
						roomHistory[roomHistoryPos] = (hitPosf - rayFrom).GetLength();
					}else{
						roomHistory[roomHistoryPos] = maxDistance * 2.f;
					}
					
					if(hit){
						bool hit2 = map->CastRay(rayFrom, -rayTo, maxDistance, hitPos);
						if(hit2)
							roomFeedbackHistory[roomHistoryPos] = 1.f;
						else
							roomFeedbackHistory[roomHistoryPos] = 0.f;
					}
					
					roomHistoryPos++;
					if(roomHistoryPos == (int)roomHistory.size())
						roomHistoryPos = 0;
				}
				
				// monte-carlo integration
				unsigned int rayHitCount = 0;
				roomVolume = 0.f;
				roomArea = 0.f;
				roomSize = 0.f;
				feedbackness = 0.f;
				for(size_t i = 0; i < roomHistory.size(); i++){
					float dist = roomHistory[i];
					if(dist < maxDistance){
						rayHitCount++;
						roomVolume += dist * dist;
						roomArea += dist;
						roomSize += dist;
					}
					
					feedbackness += roomFeedbackHistory[i];
				}
				
				if(rayHitCount > roomHistory.size() / 4){
					roomVolume /= (float)rayHitCount;
					roomVolume *= 4.f / 3.f * static_cast<float>(M_PI);
					roomArea /= (float)rayHitCount;
					roomArea *= 4.f * static_cast<float>(M_PI);
					roomSize /= (float)rayHitCount;
					reflections = (float)rayHitCount / (float)roomHistory.size();
				}else{
					roomVolume = 8.f;
					reflections = 0.2f;
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
		
		static YsrContext::PlayParam TranslateParam(const client::AudioParam& base) {
			YsrContext::PlayParam param;
			param.pitch = base.pitch;
			param.referenceDistance = base.referenceDistance;
			param.volume = base.volume;
			return param;
		}
		
		void YsrDevice::Play(client::IAudioChunk *c, const Vector3& origin, const client::AudioParam& param) {
			SPADES_MARK_FUNCTION();
			
			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if(chunk == nullptr) SPRaise("Invalid chunk: null or invalid type.");
			
			driver->PlayAbsolute(chunk->GetBuffer(), origin,
								 TranslateParam(param));
		}
		void YsrDevice::PlayLocal(client::IAudioChunk *c, const Vector3& origin, const client::AudioParam& param) {
			SPADES_MARK_FUNCTION();
			
			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if(chunk == nullptr) SPRaise("Invalid chunk: null or invalid type.");
			
			driver->PlayRelative(chunk->GetBuffer(), origin,
								 TranslateParam(param));
			
		}
		void YsrDevice::PlayLocal(client::IAudioChunk *c, const client::AudioParam& param) {
			SPADES_MARK_FUNCTION();
			
			auto *chunk = dynamic_cast<YsrAudioChunk *>(c);
			if(chunk == nullptr) SPRaise("Invalid chunk: null or invalid type.");
			
			driver->PlayLocal(chunk->GetBuffer(), TranslateParam(param));
			
		}
		
	}
}

