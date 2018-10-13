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

#include "ScriptManager.h"
#include <Gui/PackageUpdateManager.h>

namespace spades{
	namespace client {

		class PackageUpdateManagerRegistrar : public ScriptObjectRegistrar {
			static void VersionInfoFactory(PackageUpdateManager::VersionInfo *p) {
				new(p) PackageUpdateManager::VersionInfo{};
			}
			static void VersionInfoFactory2(const PackageUpdateManager::VersionInfo& other, PackageUpdateManager::VersionInfo *p) {
				new(p) PackageUpdateManager::VersionInfo(other);
			}
			static void VersionInfoDestructor(PackageUpdateManager::VersionInfo *p) {
				p->~VersionInfo();
			}
		public:
			PackageUpdateManagerRegistrar() : ScriptObjectRegistrar("PackageUpdateManager") {}
			void Register(ScriptManager *manager, Phase phase) override {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("PackageUpdateManager",
													0, asOBJ_REF | asOBJ_NOCOUNT);
						manager->CheckError(r);
						r = eng->RegisterObjectType("PackageUpdateManagerVersionInfo",
													sizeof(PackageUpdateManager::VersionInfo),
													asOBJ_VALUE | asGetTypeTraits<PackageUpdateManager::VersionInfo>());
						manager->CheckError(r);
						r = eng->RegisterEnum("PackageUpdateManagerReadyState");
						manager->CheckError(r);

						break;
					case PhaseObjectMember:
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "PackageUpdateManagerReadyState get_UpdateInfoReadyState()",
													  asMETHOD(PackageUpdateManager, GetUpdateInfoReadyState),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "bool get_UpdateAvailable()",
													  asMETHOD(PackageUpdateManager, IsUpdateAvailable),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "PackageUpdateManagerVersionInfo get_LatestVersionInfo()",
													  asMETHOD(PackageUpdateManager, GetLatestVersionInfo),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "PackageUpdateManagerVersionInfo get_CurrentVersionInfo()",
													  asMETHOD(PackageUpdateManager, GetCurrentVersionInfo),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "string get_LatestVersionInfoPageURL()",
													  asMETHOD(PackageUpdateManager, GetLatestVersionInfoPageURL),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManager",
													  "void CheckForUpdate()",
													  asMETHOD(PackageUpdateManager, CheckForUpdate),
													  asCALL_THISCALL);
						manager->CheckError(r);
						
						r = eng->RegisterObjectBehaviour("PackageUpdateManagerVersionInfo",
														 asBEHAVE_CONSTRUCT,
														 "void f()",
														 asFUNCTION(VersionInfoFactory),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("PackageUpdateManagerVersionInfo",
														 asBEHAVE_CONSTRUCT,
														 "void f(const DynamicLightParam& in)",
														 asFUNCTION(VersionInfoFactory2),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("PackageUpdateManagerVersionInfo",
														 asBEHAVE_DESTRUCT,
														 "void f()",
														 asFUNCTION(VersionInfoDestructor),
														 asCALL_CDECL_OBJLAST);
						manager->CheckError(r);
						r = eng->RegisterObjectMethod("PackageUpdateManagerVersionInfo",
													  "string ToString()",
													  asMETHOD(PackageUpdateManager::VersionInfo, ToString),
													  asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("PackageUpdateManagerVersionInfo",
														"int Major",
														asOFFSET(PackageUpdateManager::VersionInfo, major));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("PackageUpdateManagerVersionInfo",
														"int Minor",
														asOFFSET(PackageUpdateManager::VersionInfo, minor));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("PackageUpdateManagerVersionInfo",
														"int Revision",
														asOFFSET(PackageUpdateManager::VersionInfo, revision));
						manager->CheckError(r);
						r = eng->RegisterObjectProperty("PackageUpdateManagerVersionInfo",
														"string Text",
														asOFFSET(PackageUpdateManager::VersionInfo, text));
						manager->CheckError(r);
						
						r = eng->RegisterEnumValue("PackageUpdateManagerReadyState", "NotLoaded",
												   (int)PackageUpdateManager::ReadyState::NotLoaded);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("PackageUpdateManagerReadyState", "Loading",
												   (int)PackageUpdateManager::ReadyState::Loading);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("PackageUpdateManagerReadyState", "Loaded",
												   (int)PackageUpdateManager::ReadyState::Loaded);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("PackageUpdateManagerReadyState", "Error",
												   (int)PackageUpdateManager::ReadyState::Error);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("PackageUpdateManagerReadyState", "Unavailable",
												   (int)PackageUpdateManager::ReadyState::Unavailable);
						manager->CheckError(r);
						break;
					default:

						break;
				}
			}
		};

		static PackageUpdateManagerRegistrar registrar;
	}
}
