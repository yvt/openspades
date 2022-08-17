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

#include <cstring>
#include <exception>
#include <cstdlib>
#include <vector>
#include <utility>

#include "ALDevice.h"
#include "ALFuncs.h"
#include <Client/GameMap.h>
#include <Client/IAudioChunk.h>
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/FileManager.h>
#include <Core/IAudioStream.h>
#include <Core/Settings.h>
#include <Core/AudioStream.h>

DEFINE_SPADES_SETTING(s_maxPolyphonics, "96");
DEFINE_SPADES_SETTING(s_eax, "1");
DEFINE_SPADES_SETTING(s_alPreciseErrorCheck, "1");
DEFINE_SPADES_SETTING(s_gain, "1");
DEFINE_SPADES_SETTING(s_openalDevice, "");

// lm: seems to be missing for me..
#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
#endif

#define ALCheckErrorPrecise()                                                                      \
	if (s_alPreciseErrorCheck)                                                                     \
	ALCheckError()

namespace spades {
	namespace audio {

		static Vector3 TransformVectorToAL(Vector3 v) { return MakeVector3(v.x, v.y, v.z); }
		static Vector3 TransformVectorFromAL(Vector3 v) { return MakeVector3(v.x, v.y, v.z); }

		namespace {
			std::vector<uint8_t> ConvertFloatBufferToSignedShort(const std::vector<uint8_t> &bytes) {
				if (bytes.size() & 3) {
					SPRaise("Size is invalid");
				}
				std::vector<uint8_t> ret;
				ret.resize(bytes.size() >> 1);

				for (size_t i = 0; i < bytes.size(); i += 4) {
					float inValue = reinterpret_cast<const float *>(bytes.data())[i >> 2];
					int16_t &outValue = reinterpret_cast<std::int16_t *>(ret.data())[i >> 2];
					outValue = static_cast<int16_t>(std::floor(inValue * 32768.f));
				}

				return ret;
			}
		}

		class ALAudioChunk : public client::IAudioChunk {
			ALuint handle;
			ALuint format;

		protected:
			virtual ~ALAudioChunk() {
				SPADES_MARK_FUNCTION();

				al::qalDeleteBuffers(1, &handle);
				ALCheckErrorPrecise();
			}

		public:
			ALuint GetHandle() { return handle; }
			ALAudioChunk(IAudioStream *audioStream) {
				SPADES_MARK_FUNCTION();

				std::vector<uint8_t> bytes;
				if (audioStream->GetLength() > 128 * 1024 * 1024) {
					SPRaise("Audio stream too long");
				}

				size_t len = (size_t)audioStream->GetLength();
				bytes.resize(len);

				audioStream->SetPosition(0);
				if (audioStream->Read(bytes.data(), len) < len)
					SPRaise("Failed to read audio data");

				ALuint alFormat;
				switch (audioStream->GetSampleFormat()) {
					case IAudioStream::UnsignedByte:
						switch (audioStream->GetNumChannels()) {
							case 1: alFormat = AL_FORMAT_MONO8; break;
							case 2: alFormat = AL_FORMAT_STEREO8; break;
							default: SPRaise("Unsupported audio format");
						}
						break;
					case IAudioStream::SignedShort:
						switch (audioStream->GetNumChannels()) {
							case 1: alFormat = AL_FORMAT_MONO16; break;
							case 2: alFormat = AL_FORMAT_STEREO16; break;
							default: SPRaise("Unsupported audio format");
						}
						break;
					case IAudioStream::SingleFloat:
						bytes = ConvertFloatBufferToSignedShort(bytes);
						switch (audioStream->GetNumChannels()) {
							case 1: alFormat = AL_FORMAT_MONO16; break;
							case 2: alFormat = AL_FORMAT_STEREO16; break;
							default: SPRaise("Unsupported audio format");
						}
						break;
					default: SPRaise("Unsupported audio format");
				}

				format = alFormat;

				al::qalGenBuffers(1, &handle);
				ALCheckError();
				al::qalBufferData(handle, alFormat, bytes.data(), (ALuint)bytes.size(),
				                  audioStream->GetSamplingFrequency());
				ALCheckError();
			}

			ALuint GetFormat() { return format; }
		};

		class ALDevice::Internal {
		public:
			bool useEAX;
			ALCdevice *alDevice;
			ALCcontext *alContext;

			ALuint reverbFXSlot;
			ALuint reverbFX;

			ALuint obstructionFilter;

			client::GameMap *map;

			struct ALSrc {
				Internal *internal;
				ALuint handle;
				bool eaxSource;
				bool stereo;
				bool local;
				client::AudioParam param;

				ALSrc(Internal *i) : internal(i) {
					SPADES_MARK_FUNCTION();

					al::qalGenSources(1, &handle);
					ALCheckError();
				}

				~ALSrc() {
					SPADES_MARK_FUNCTION();

					al::qalDeleteSources(1, &handle);
					ALCheckErrorPrecise();
				}

				void Terminate() {
					SPADES_MARK_FUNCTION();

					al::qalSourceStop(handle);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_BUFFER, 0);
					ALCheckError();
				}

				bool IsPlaying() {
					SPADES_MARK_FUNCTION();

					ALint value;
					al::qalGetSourcei(handle, AL_BUFFER, &value);
					ALCheckError();
					if (value == 0)
						return false;

					al::qalGetSourcei(handle, AL_SOURCE_STATE, &value);
					ALCheckError();
					if (value == AL_STOPPED)
						return false;
					return true;
				}

				// must be called
				void SetParam(const client::AudioParam &param) {
					SPADES_MARK_FUNCTION();

					al::qalSourcef(handle, AL_PITCH, param.pitch);
					ALCheckErrorPrecise();
					al::qalSourcef(handle, AL_GAIN, param.volume * std::max<float>(std::min<float>(s_gain, 4.0f), 0.0f));
					ALCheckErrorPrecise();
					al::qalSourcef(handle, AL_REFERENCE_DISTANCE, param.referenceDistance);

					ALCheckError();
					this->param = param;
				}

				// either Set3D or Set2D must be called
				void Set3D(const Vector3 &v, bool local = false) {
					SPADES_MARK_FUNCTION();

					ALfloat pos[] = {v.x, v.y, v.z};
					ALfloat vel[] = {0, 0, 0};
					al::qalSourcefv(handle, AL_POSITION, pos);
					ALCheckErrorPrecise();
					al::qalSourcefv(handle, AL_VELOCITY, vel);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_SOURCE_RELATIVE, local ? AL_TRUE : AL_FALSE);
					ALCheckErrorPrecise();
					al::qalSourcef(handle, AL_ROLLOFF_FACTOR, (local || stereo) ? 0.f : 1.f);
					ALCheckErrorPrecise();
					if (internal->useEAX) {
						al::qalSource3i(handle, AL_AUXILIARY_SEND_FILTER, internal->reverbFXSlot, 0,
						                AL_FILTER_NULL);
						ALCheckErrorPrecise();
					}
					eaxSource = true;
					this->local = local;
					ALCheckError();
				}

				void Set2D() {
					SPADES_MARK_FUNCTION();

					ALfloat pos[] = {0, 0, 0};
					ALfloat vel[] = {0, 0, 0};
					al::qalSourcefv(handle, AL_POSITION, pos);
					ALCheckErrorPrecise();
					al::qalSourcefv(handle, AL_VELOCITY, vel);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_SOURCE_RELATIVE, AL_TRUE);
					ALCheckErrorPrecise();
					al::qalSourcef(handle, AL_ROLLOFF_FACTOR, 0.f);
					ALCheckErrorPrecise();
					if (internal->useEAX) {
						al::qalSource3i(handle, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0,
						                AL_FILTER_NULL);
						ALCheckErrorPrecise();
						al::qalSourcei(handle, AL_DIRECT_FILTER, 0);
						ALCheckErrorPrecise();
					}
					ALCheckError();
					eaxSource = false;
					local = true;
				}

				// after calling Set2D/Set3D, must be called
				void UpdateObstruction() {
					SPADES_MARK_FUNCTION();

					// update stereo source's volume (not spatialized by AL)
					// FIXME: move to another function?
					if (stereo && !local) {
						ALfloat v3[3];
						al::qalGetListenerfv(AL_POSITION, v3);
						Vector3 eye = {v3[0], v3[1], v3[2]};
						ALCheckErrorPrecise();
						al::qalGetSourcefv(handle, AL_POSITION, v3);
						Vector3 pos = {v3[0], v3[1], v3[2]};
						ALCheckErrorPrecise();

						float dist = (pos - eye).GetLength();
						dist /= param.referenceDistance;
						if (dist < 1.f)
							dist = 1.f;
						dist = 1.f / dist;
						al::qalSourcef(handle, AL_GAIN, param.volume * dist);
						ALCheckError();
					}

					if (!internal->useEAX)
						return;

					ALint value;

					al::qalGetSourcei(handle, AL_SOURCE_RELATIVE, &value);
					ALCheckErrorPrecise();

					bool enableObstruction = true;
					if (value)
						enableObstruction = false;

					// raytrace
					client::GameMap *map = internal->map;
					if (map && enableObstruction) {
						ALfloat v3[3];
						al::qalGetListenerfv(AL_POSITION, v3);
						Vector3 eye = {v3[0], v3[1], v3[2]};
						ALCheckErrorPrecise();
						al::qalGetSourcefv(handle, AL_POSITION, v3);
						Vector3 pos = {v3[0], v3[1], v3[2]};
						ALCheckErrorPrecise();
						Vector3 checkPos;
						eye = TransformVectorFromAL(eye);
						pos = TransformVectorFromAL(pos);
						for (int x = -1; x <= 1; x++)
							for (int y = -1; y <= 1; y++)
								for (int z = -1; z <= 1; z++) {
									IntVector3 hitPos;
									checkPos.x = pos.x + (float)x * .2f;
									checkPos.y = pos.y + (float)y * .2f;
									checkPos.z = pos.z + (float)z * .2f;
									if (!map->CastRay(eye, (checkPos - eye).Normalize(),
									                  (checkPos - eye).GetLength(), hitPos)) {
										enableObstruction = false;
									}
								}
					} else {
						enableObstruction = false;
					}

					ALuint fx = AL_EFFECTSLOT_NULL;
					ALuint flt = AL_FILTER_NULL;

					if (enableObstruction)
						flt = internal->obstructionFilter;

					if (eaxSource)
						fx = internal->reverbFXSlot;

					al::qalSource3i(handle, AL_AUXILIARY_SEND_FILTER, fx, 0, flt);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_DIRECT_FILTER, flt);
					ALCheckError();
				}

				void PlayBufferOneShot(ALuint buffer) {
					SPADES_MARK_FUNCTION();

					al::qalSourcei(handle, AL_LOOPING, AL_FALSE);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_BUFFER, buffer);
					ALCheckErrorPrecise();
					al::qalSourcei(handle, AL_SAMPLE_OFFSET, 0);
					ALCheckErrorPrecise();
					al::qalSourcePlay(handle);
					ALCheckError();
				}
			};

			std::vector<ALSrc *> srcs;

			// reverb simulator
			int roomHistoryPos;
			std::vector<float> roomHistory;
			std::vector<float> roomFeedbackHistory;

			void updateEFXReverb(LPEFXEAXREVERBPROPERTIES reverb) {
				SPADES_MARK_FUNCTION_DEBUG();

				al::qalEffectf(reverbFX, AL_EAXREVERB_DENSITY, reverb->flDensity);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_DIFFUSION, reverb->flDiffusion);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_GAIN, reverb->flGain);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_GAINHF, reverb->flGainHF);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_GAINLF, reverb->flGainLF);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_DECAY_TIME, reverb->flDecayTime);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_DECAY_LFRATIO, reverb->flDecayLFRatio);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_REFLECTIONS_DELAY,
				               reverb->flReflectionsDelay);
				ALCheckErrorPrecise();
				al::qalEffectfv(reverbFX, AL_EAXREVERB_REFLECTIONS_PAN, reverb->flReflectionsPan);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
				ALCheckErrorPrecise();
				al::qalEffectfv(reverbFX, AL_EAXREVERB_LATE_REVERB_PAN, reverb->flLateReverbPan);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_ECHO_TIME, reverb->flEchoTime);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_ECHO_DEPTH, reverb->flEchoDepth);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_MODULATION_TIME, reverb->flModulationTime);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_MODULATION_DEPTH, reverb->flModulationDepth);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_AIR_ABSORPTION_GAINHF,
				               reverb->flAirAbsorptionGainHF);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_HFREFERENCE, reverb->flHFReference);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_LFREFERENCE, reverb->flLFReference);
				ALCheckErrorPrecise();
				al::qalEffectf(reverbFX, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR,
				               reverb->flRoomRolloffFactor);
				ALCheckErrorPrecise();
				al::qalEffecti(reverbFX, AL_EAXREVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);
				ALCheckErrorPrecise();
			}

			Internal() {
				SPADES_MARK_FUNCTION();
				const char *ext, *dev;

				if (al::qalGetString(AL_EXTENSIONS)) {
					std::vector<std::string> strs = Split(al::qalGetString(AL_EXTENSIONS), " ");
					SPLog("OpenAL Extensions:");
					for (size_t i = 0; i < strs.size(); i++) {
						SPLog("  %s", strs[i].c_str());
					}
				}

				dev = s_openalDevice.CString();
				SPLog("OpenAL opening device: %s", dev);
				if (!strcmp(dev, "default"))
	                          dev = NULL;
				alDevice = al::qalcOpenDevice(dev);

				if (UNLIKELY(!alDevice)) {
					if ((ext = al::qalcGetString(NULL, ALC_EXTENSIONS))) {
						std::vector<std::string> strs = Split(ext, " ");
						SPLog("OpenAL ALC Extensions (NULL):");
						for (size_t i = 0; i < strs.size(); i++) {
							SPLog("  %s", strs[i].c_str());
						}
					}

					SPRaise("Failed to open OpenAL device.");
				}
				SPLog("OpenAL Info:");
				SPLog("  Vendor: %s", al::qalGetString(AL_VENDOR));
				SPLog("  Version: %s", al::qalGetString(AL_VERSION));
				SPLog("  Renderer: %s", al::qalGetString(AL_RENDERER));

				if ((ext = al::qalcGetString(alDevice, ALC_EXTENSIONS))) {
					std::vector<std::string> strs = Split(ext, " ");
					SPLog("OpenAL ALC Extensions:");
					for (size_t i = 0; i < strs.size(); i++) {
						SPLog("  %s", strs[i].c_str());
					}
				}

				alContext = al::qalcCreateContext(alDevice, NULL);
				if (UNLIKELY(!alContext)) {
					al::qalcCloseDevice(alDevice);
					SPRaise("Failed to create OpenAL context.");
				}

				al::qalcMakeContextCurrent(alContext);

				map = NULL;
				roomHistoryPos = 0;

				if (s_eax) {
					try {
						al::InitEAX();
						useEAX = true;
						SPLog("EAX enabled");
					} catch (const std::exception &ex) {
						useEAX = false;
						s_eax = 0;
						SPLog("Failed to initialize EAX: \n%s", ex.what());
					}
				} else {
					SPLog("EAX is disabled by configuration (s_eax)");
					useEAX = false;
				}

				// somehow Apple OpenAL reports an error whose source is unknown...
				// so we clear that now
				while (al::qalGetError() != AL_NO_ERROR)
					;

				ALCheckError();

				for (int i = 0; i < (int)s_maxPolyphonics; i++) {
					srcs.push_back(new ALSrc(this));
				}

				SPLog("%d source(s) initialized", (int)s_maxPolyphonics);

				al::qalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
				ALCheckErrorPrecise();

				// initialize EAX
				if (useEAX) {
					al::qalGenAuxiliaryEffectSlots(1, &reverbFXSlot);
					ALCheckError();
					al::qalGenEffects(1, &reverbFX);
					ALCheckError();

					al::qalEffecti(reverbFX, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
					ALCheckError();

					EFXEAXREVERBPROPERTIES prop = EFX_REVERB_PRESET_MOUNTAINS;
					updateEFXReverb(&prop);

					al::qalAuxiliaryEffectSloti(reverbFXSlot, AL_EFFECTSLOT_EFFECT, reverbFX);
					ALCheckError();

					al::qalGenFilters(1, &obstructionFilter);
					ALCheckErrorPrecise();
					al::qalFilteri(obstructionFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
					ALCheckErrorPrecise();
					al::qalFilterf(obstructionFilter, AL_LOWPASS_GAIN, 1.f);
					ALCheckErrorPrecise();
					al::qalFilterf(obstructionFilter, AL_LOWPASS_GAINHF, 0.1f);
					ALCheckErrorPrecise();

					for (int i = 0; i < 128; i++) {
						roomHistory.push_back(20000.f);
						roomFeedbackHistory.push_back(0.f);
					}
				}
			}
			~Internal() {
				SPADES_MARK_FUNCTION();

				for (size_t i = 0; i < srcs.size(); i++)
					delete srcs[i];
			}

			ALSrc *AllocChunk() {
				SPADES_MARK_FUNCTION();

				size_t start = SampleRandomInt<std::size_t>(0, srcs.size() - 1);
				for (size_t i = 0; i < srcs.size(); i++) {
					ALSrc *src = srcs[(i + start) % srcs.size()];
					if (src->IsPlaying())
						continue;
					return src;
				}

                		ALSrc *src = SampleRandomElement(srcs);
				src->Terminate();
				return src;
			}

			void Play(ALAudioChunk *chunk, const Vector3 &origin, const client::AudioParam &param) {
				SPADES_MARK_FUNCTION();

				ALSrc *src = AllocChunk();
				if (UNLIKELY(!src))
					return;

				src->stereo = chunk->GetFormat() == AL_FORMAT_STEREO16;
				src->SetParam(param);
				src->Set3D(origin);
				src->UpdateObstruction();
				src->PlayBufferOneShot(chunk->GetHandle());
			}
			void PlayLocal(ALAudioChunk *chunk, const Vector3 &origin,
			               const client::AudioParam &param) {
				SPADES_MARK_FUNCTION();

				ALSrc *src = AllocChunk();
				if (!src)
					return;

				src->stereo = chunk->GetFormat() == AL_FORMAT_STEREO16;
				src->SetParam(param);
				src->Set3D(origin, true);
				src->UpdateObstruction();
				src->PlayBufferOneShot(chunk->GetHandle());
			}
			void PlayLocal(ALAudioChunk *chunk, const client::AudioParam &param) {
				SPADES_MARK_FUNCTION();

				ALSrc *src = AllocChunk();
				if (!src)
					return;

				src->stereo = chunk->GetFormat() == AL_FORMAT_STEREO16;
				src->SetParam(param);
				src->Set2D();
				src->UpdateObstruction();
				src->PlayBufferOneShot(chunk->GetHandle());
			}

			void Respatialize(const Vector3 &eye, const Vector3 &front, const Vector3 &up) {
				SPADES_MARK_FUNCTION();

				float pos[] = {eye.x, eye.y, eye.z};
				float vel[] = {0, 0, 0};
				float orient[] = {front.x, front.y, front.z, up.x, up.y, up.z};

				al::qalListenerfv(AL_POSITION, pos);
				ALCheckError();
				al::qalListenerfv(AL_VELOCITY, vel);
				ALCheckError();
				al::qalListenerfv(AL_ORIENTATION, orient);
				ALCheckError();

				// do reverb simulation
				if (useEAX) {
					float maxDistance = 40.f;
					float reflections;
					float roomVolume;
					float roomArea;
					float roomSize;
					float feedbackness = 0.f;
					if (map == NULL) {
						reflections = 0.f;
						roomVolume = 1.f;
						roomArea = 1.f;
						roomSize = 10.f;
					} else {
						// do raycast
						Vector3 rayFrom = TransformVectorFromAL(eye);
						Vector3 rayTo;

						for (int rays = 0; rays < 4; rays++) {
							rayTo.x = SampleRandomFloat() - SampleRandomFloat();
							rayTo.y = SampleRandomFloat() - SampleRandomFloat();
							rayTo.z = SampleRandomFloat() - SampleRandomFloat();
							rayTo = rayTo.Normalize();

							IntVector3 hitPos;
							bool hit = map->CastRay(rayFrom, rayTo, maxDistance, hitPos);
							if (hit) {
								Vector3 hitPosf = {(float)hitPos.x, (float)hitPos.y,
								                   (float)hitPos.z};
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
							reflections = 0.2f;
							roomArea = 100.f;
							roomSize = 100.f;
						}

						feedbackness /= (float)roomHistory.size();
					}

					// printf("room size: %f, ref: %f, fb: %f\n", roomSize, reflections,
					// feedbackness);

					EFXEAXREVERBPROPERTIES prop = EFX_REVERB_PRESET_ROOM;
					prop.flDecayTime = .161f * roomVolume / roomArea / .4f;
					prop.flGain = reflections * 0.8f;
					if (prop.flDecayTime > AL_EAXREVERB_MAX_DECAY_TIME)
						prop.flDecayTime = AL_EAXREVERB_MAX_DECAY_TIME;
					if (prop.flDecayTime < AL_EAXREVERB_MIN_DECAY_TIME)
						prop.flDecayTime = AL_EAXREVERB_MIN_DECAY_TIME;
					prop.flLateReverbDelay = roomSize / 324.f;
					if (prop.flLateReverbDelay > AL_EAXREVERB_MAX_LATE_REVERB_DELAY)
						prop.flLateReverbDelay = AL_EAXREVERB_MAX_LATE_REVERB_DELAY;
					prop.flReflectionsDelay = roomSize / 324.f;
					if (prop.flReflectionsDelay > AL_EAXREVERB_MAX_REFLECTIONS_DELAY)
						prop.flReflectionsDelay = AL_EAXREVERB_MAX_REFLECTIONS_DELAY;
					prop.flLateReverbGain = reflections * reflections * reflections * reflections *
					                        feedbackness * feedbackness * feedbackness * .34f;
					prop.flReflectionsGain = reflections * .25f;

					// printf("late: %f, ref: %f\n", prop.flLateReverbGain, prop.flReflectionsGain);

					updateEFXReverb(&prop);

					al::qalAuxiliaryEffectSloti(reverbFXSlot, AL_EFFECTSLOT_EFFECT, reverbFX);
					ALCheckError();
				}

				for (size_t i = 0; i < srcs.size(); i++) {
					ALSrc *s = srcs[i];
					if ((SampleRandomInt(0, 7) == 0) && s->IsPlaying())
						s->UpdateObstruction();
				}
			}
		};

		ALDevice::ALDevice() {
			SPADES_MARK_FUNCTION();

			try {
				al::Link();
			} catch (...) {
				throw;
			}
			d = new Internal();
		}

		std::vector<std::string> ALDevice::DeviceList() {
			std::vector<std::string> devs;
			const ALCchar *ext = al::qalcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
			if (!ext || *ext == '\0')
				ext = al::qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
			while (ext && *ext) {
				devs.push_back(ext);
				ext += std::strlen(ext) + 1;
			}

			return devs;
		}

		bool ALDevice::TryLoad() {
			try {
				al::Link();
				return true;
			} catch (...) {
				return false;
			}
		}

		ALDevice::~ALDevice() {
			SPADES_MARK_FUNCTION();

			// TODO: Fix memory leak!

			delete d;
		}
		ALAudioChunk *ALDevice::CreateChunk(const char *name) {
			SPADES_MARK_FUNCTION();

			IAudioStream *as = OpenAudioStream(name);

			try {
				ALAudioChunk *ch = new ALAudioChunk(as);
				delete as;
				return ch;
			} catch (...) {
				delete as;
				throw;
			}
		}

		client::IAudioChunk *ALDevice::RegisterSound(const char *name) {
			SPADES_MARK_FUNCTION();

			std::map<std::string, ALAudioChunk *>::iterator it = chunks.find(name);
			if (it == chunks.end()) {
				ALAudioChunk *c = CreateChunk(name);
				chunks[name] = c;
				// release audiochunk later (to eliminate memory leak)
				return c;
			}
			it->second->AddRef();
			return it->second;
		}

		void ALDevice::ClearCache() {
			SPADES_MARK_FUNCTION();

			for (auto &chunk: chunks) {
				chunk.second->Release();
			}
			chunks.clear();
		}

		void ALDevice::Play(client::IAudioChunk *chunk, const spades::Vector3 &origin,
		                    const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			d->Play(static_cast<ALAudioChunk *>(chunk), TransformVectorToAL(origin), param);
		}
		void ALDevice::PlayLocal(client::IAudioChunk *chunk, const spades::Vector3 &origin,
		                         const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			d->PlayLocal(static_cast<ALAudioChunk *>(chunk),
			             MakeVector3(origin.x, origin.y, -origin.z), param);
		}

		void ALDevice::PlayLocal(client::IAudioChunk *chunk, const client::AudioParam &param) {
			SPADES_MARK_FUNCTION();

			d->PlayLocal(static_cast<ALAudioChunk *>(chunk), param);
		}

		void ALDevice::Respatialize(const spades::Vector3 &eye, const spades::Vector3 &front,
		                            const spades::Vector3 &up) {
			SPADES_MARK_FUNCTION();

			d->Respatialize(TransformVectorToAL(eye), TransformVectorToAL(front),
			                TransformVectorToAL(up));
		}

		void ALDevice::SetGameMap(client::GameMap *mp) {
			SPADES_MARK_FUNCTION_DEBUG();
			client::GameMap *oldMap = d->map;
			d->map = mp;
			if (mp)
				mp->AddRef();
			if (oldMap)
				oldMap->Release();
		}
	}
}
