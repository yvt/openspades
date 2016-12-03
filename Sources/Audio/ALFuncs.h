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
#define AL_NO_PROTOTYPES
#define ALC_NO_PROTOTYPES

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx-creative.h"
#include "AL/efx-presets.h"
#include "AL/efx.h"

#include "AL/MacOSX_OALExtensions.h"

namespace al {
	extern LPALENABLE qalEnable;
	extern LPALDISABLE qalDisable;
	extern LPALISENABLED qalIsEnabled;
	extern LPALGETSTRING qalGetString;
	extern LPALGETBOOLEANV qalGetBooleanv;
	extern LPALGETINTEGERV qalGetIntegerv;
	extern LPALGETFLOATV qalGetFloatv;
	extern LPALGETDOUBLEV qalGetDoublev;
	extern LPALGETBOOLEAN qalGetBoolean;
	extern LPALGETINTEGER qalGetInteger;
	extern LPALGETFLOAT qalGetFloat;
	extern LPALGETDOUBLE qalGetDouble;
	extern LPALGETERROR qalGetError;
	extern LPALISEXTENSIONPRESENT qalIsExtensionPresent;
	extern LPALGETPROCADDRESS qalGetProcAddress;
	extern LPALGETENUMVALUE qalGetEnumValue;
	extern LPALLISTENERF qalListenerf;
	extern LPALLISTENER3F qalListener3f;
	extern LPALLISTENERFV qalListenerfv;
	extern LPALLISTENERI qalListeneri;
	extern LPALLISTENER3I qalListener3i;
	extern LPALLISTENERIV qalListeneriv;
	extern LPALGETLISTENERF qalGetListenerf;
	extern LPALGETLISTENER3F qalGetListener3f;
	extern LPALGETLISTENERFV qalGetListenerfv;
	extern LPALGETLISTENERI qalGetListeneri;
	extern LPALGETLISTENER3I qalGetListener3i;
	extern LPALGETLISTENERIV qalGetListeneriv;
	extern LPALGENSOURCES qalGenSources;
	extern LPALDELETESOURCES qalDeleteSources;
	extern LPALISSOURCE qalIsSource;
	extern LPALSOURCEF qalSourcef;
	extern LPALSOURCE3F qalSource3f;
	extern LPALSOURCEFV qalSourcefv;
	extern LPALSOURCEI qalSourcei;
	extern LPALSOURCE3I qalSource3i;
	extern LPALSOURCEIV qalSourceiv;
	extern LPALGETSOURCEF qalGetSourcef;
	extern LPALGETSOURCE3F qalGetSource3f;
	extern LPALGETSOURCEFV qalGetSourcefv;
	extern LPALGETSOURCEI qalGetSourcei;
	extern LPALGETSOURCE3I qalGetSource3i;
	extern LPALGETSOURCEIV qalGetSourceiv;
	extern LPALSOURCEPLAYV qalSourcePlayv;
	extern LPALSOURCESTOPV qalSourceStopv;
	extern LPALSOURCEREWINDV qalSourceRewindv;
	extern LPALSOURCEPAUSEV qalSourcePausev;
	extern LPALSOURCEPLAY qalSourcePlay;
	extern LPALSOURCESTOP qalSourceStop;
	extern LPALSOURCEREWIND qalSourceRewind;
	extern LPALSOURCEPAUSE qalSourcePause;
	extern LPALSOURCEQUEUEBUFFERS qalSourceQueueBuffers;
	extern LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
	extern LPALGENBUFFERS qalGenBuffers;
	extern LPALDELETEBUFFERS qalDeleteBuffers;
	extern LPALISBUFFER qalIsBuffer;
	extern LPALBUFFERDATA qalBufferData;
	extern LPALBUFFERF qalBufferf;
	extern LPALBUFFER3F qalBuffer3f;
	extern LPALBUFFERFV qalBufferfv;
	extern LPALBUFFERF qalBufferi;
	extern LPALBUFFER3F qalBuffer3i;
	extern LPALBUFFERFV qalBufferiv;
	extern LPALGETBUFFERF qalGetBufferf;
	extern LPALGETBUFFER3F qalGetBuffer3f;
	extern LPALGETBUFFERFV qalGetBufferfv;
	extern LPALGETBUFFERI qalGetBufferi;
	extern LPALGETBUFFER3I qalGetBuffer3i;
	extern LPALGETBUFFERIV qalGetBufferiv;
	extern LPALDOPPLERFACTOR qalDopplerFactor;
	extern LPALDOPPLERVELOCITY qalDopplerVelocity;
	extern LPALSPEEDOFSOUND qalSpeedOfSound;
	extern LPALDISTANCEMODEL qalDistanceModel;

	// EAX
	extern LPALGENEFFECTS qalGenEffects;
	extern LPALDELETEEFFECTS qalDeleteEffects;
	extern LPALISEFFECT qalIsEffect;
	extern LPALEFFECTI qalEffecti;
	extern LPALEFFECTIV qalEffectiv;
	extern LPALEFFECTF qalEffectf;
	extern LPALEFFECTFV qalEffectfv;
	extern LPALGETEFFECTI qalGetEffecti;
	extern LPALGETEFFECTIV qalGetEffectiv;
	extern LPALGETEFFECTF qalGetEffectf;
	extern LPALGETEFFECTFV qalGetEffectfv;
	extern LPALGENFILTERS qalGenFilters;
	extern LPALDELETEFILTERS qalDeleteFilters;
	extern LPALISFILTER qalIsFilter;
	extern LPALFILTERI qalFilteri;
	extern LPALFILTERIV qalFilteriv;
	extern LPALFILTERF qalFilterf;
	extern LPALFILTERFV qalFilterfv;
	extern LPALGETFILTERI qalGetFilteri;
	extern LPALGETFILTERIV qalGetFilteriv;
	extern LPALGETFILTERF qalGetFilterf;
	extern LPALGETFILTERFV qalGetFilterfv;
	extern LPALGENAUXILIARYEFFECTSLOTS qalGenAuxiliaryEffectSlots;
	extern LPALDELETEAUXILIARYEFFECTSLOTS qalDeleteAuxiliaryEffectSlots;
	extern LPALISAUXILIARYEFFECTSLOT qalIsAuxiliaryEffectSlot;
	extern LPALAUXILIARYEFFECTSLOTI qalAuxiliaryEffectSloti;
	extern LPALAUXILIARYEFFECTSLOTIV qalAuxiliaryEffectSlotiv;
	extern LPALAUXILIARYEFFECTSLOTF qalAuxiliaryEffectSlotf;
	extern LPALAUXILIARYEFFECTSLOTFV qalAuxiliaryEffectSlotfv;
	extern LPALGETAUXILIARYEFFECTSLOTI qalGetAuxiliaryEffectSloti;
	extern LPALGETAUXILIARYEFFECTSLOTIV qalGetAuxiliaryEffectSlotiv;
	extern LPALGETAUXILIARYEFFECTSLOTF qalGetAuxiliaryEffectSlotf;
	extern LPALGETAUXILIARYEFFECTSLOTFV qalGetAuxiliaryEffectSlotfv;

	// Mac OS X Extensions
	// ALC_EXT_MAC_OSX
	extern alcMacOSXRenderingQualityProcPtr qalcMacOSXRenderingQuality;
	extern alMacOSXRenderChannelCountProcPtr qalMacOSXRenderChannelCount;
	extern alcMacOSXMixerMaxiumumBussesProcPtr qalcMacOSXMixerMaxiumumBusses;
	extern alcMacOSXMixerOutputRateProcPtr qalcMacOSXMixerOutputRate;
	extern alcMacOSXGetRenderingQualityProcPtr qalcMacOSXGetRenderingQuality;
	extern alMacOSXGetRenderChannelCountProcPtr qalMacOSXGetRenderChannelCount;
	extern alcMacOSXGetMixerMaxiumumBussesProcPtr qalcMacOSXGetMixerMaxiumumBusses;
	extern alcMacOSXGetMixerOutputRateProcPtr qalcMacOSXGetMixerOutputRate;
	// ALC_EXT_ASA
	extern alcASAGetSourceProcPtr qalcASAGetSource;
	extern alcASASetSourceProcPtr qalcASASetSource;
	extern alcASAGetListenerProcPtr qalcASAGetListener;
	extern alcASASetListenerProcPtr qalcASASetListener;

	extern LPALCCREATECONTEXT qalcCreateContext;
	extern LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
	extern LPALCPROCESSCONTEXT qalcProcessContext;
	extern LPALCSUSPENDCONTEXT qalcSuspendContext;
	extern LPALCDESTROYCONTEXT qalcDestroyContext;
	extern LPALCGETCURRENTCONTEXT qalcGetCurrentContext;
	extern LPALCGETCONTEXTSDEVICE qalcGetContextsDevice;
	extern LPALCOPENDEVICE qalcOpenDevice;
	extern LPALCCLOSEDEVICE qalcCloseDevice;
	extern LPALCGETERROR qalcGetError;
	extern LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
	extern LPALCGETPROCADDRESS qalcGetProcAddress;
	extern LPALCGETENUMVALUE qalcGetEnumValue;
	extern LPALCGETSTRING qalcGetString;
	extern LPALCGETINTEGERV qalcGetIntegerv;
	extern LPALCCAPTUREOPENDEVICE qalcCaptureOpenDevice;
	extern LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
	extern LPALCCAPTURESTART qalcCaptureStart;
	extern LPALCCAPTURESTOP qalcCaptureStop;
	extern LPALCCAPTURESAMPLES qalcCaptureSamples;

	void Link(void);
	void InitEAX(void);

	const char *DescribeError(ALenum);
	void CheckError(void);
	void CheckError(const char *source, const char *fun, int line);
}

#define ALCheckError() ::al::CheckError(__FILE__, __PRETTY_FUNCTION__, __LINE__)
