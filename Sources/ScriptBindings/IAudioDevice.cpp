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

#include "ScriptManager.h"
#include <Client/IAudioDevice.h>
#include <Client/IAudioChunk.h>

namespace spades {
	namespace client{
		
		
		
		class AudioDeviceRegistrar: public ScriptObjectRegistrar {
			static void AudioParamFactory(AudioParam *p) {
				new(p) AudioParam();
			}
			static IAudioChunk *RegisterSound(const std::string& name, IAudioDevice *dev) {
				try{
					IAudioChunk *c = dev->RegisterSound(name.c_str());
					c->AddRef();
					return c;
				}catch(const std::exception& ex) {
					ScriptContextUtils().SetNativeException(ex);
					return nullptr;
				}
			}
		public:
			AudioDeviceRegistrar():
			ScriptObjectRegistrar("AudioDevice"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("AudioDevice",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectType("AudioParam",
													sizeof(AudioParam), asOBJ_VALUE|asOBJ_POD|asOBJ_APP_CLASS_CDAK);
						manager->CheckError(r);
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterObjectBehaviour("AudioDevice",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(IAudioDevice, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("AudioDevice",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(IAudioDevice, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "AudioChunk@ RegisterSound(const string& in)",
													  asFUNCTION(RegisterSound),
													  asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "void set_GameMap(GameMap@+)",
													  asMETHOD(IAudioDevice, SetGameMap),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "void Play(AudioChunk@+, const Vector3& in, const AudioParam& in)",
													  asMETHOD(IAudioDevice, Play),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "void PlayLocal(AudioChunk@+, const Vector3& in, const AudioParam& in)",
													  asMETHODPR(IAudioDevice, PlayLocal, (IAudioChunk*, const Vector3&, const AudioParam&), void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "void PlayLocal(AudioChunk@+, const AudioParam& in)",
													  asMETHODPR(IAudioDevice, PlayLocal, (IAudioChunk*, const AudioParam&), void),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("AudioDevice",
													  "void Respatialize(const Vector3& in, const Vector3& in, const Vector3& in)",
													  asMETHOD(IAudioDevice, Respatialize),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("AudioParam", asBEHAVE_CONSTRUCT,
														 "void f()",
														 asFUNCTION(AudioParamFactory),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("AudioParam",
														"float volume",
														asOFFSET(AudioParam, volume));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("AudioParam",
														"float pitch",
														asOFFSET(AudioParam, pitch));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("AudioParam",
														"float referenceDistance",
														asOFFSET(AudioParam, referenceDistance));
						manager->CheckError(r);
						break;
					default:
						break;
				}
			}
		};
		
		static AudioDeviceRegistrar registrar;
	}
}
