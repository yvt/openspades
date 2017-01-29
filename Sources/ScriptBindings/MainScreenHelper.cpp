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
#include <Gui/MainScreenHelper.h>

namespace spades {
	class MainScreenHelperRegistrar: public ScriptObjectRegistrar {
		
		
	public:
		MainScreenHelperRegistrar():
		ScriptObjectRegistrar("MainScreenHelper") {}
		
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("MainScreenHelper",
												0, asOBJ_REF);
					manager->CheckError(r);
					
					r = eng->RegisterObjectType("MainScreenServerItem",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("MainScreenHelper",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(gui::MainScreenHelper, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("MainScreenHelper",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(gui::MainScreenHelper, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "void StartQuery()",
												  asMETHOD(gui::MainScreenHelper, StartQuery),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "bool SetServerFavorite(string, bool)",
												  asMETHOD(gui::MainScreenHelper, SetServerFavorite),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "bool PollServerListState()",
												  asMETHOD(gui::MainScreenHelper, PollServerListState),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "string GetServerListQueryMessage()",
												  asMETHOD(gui::MainScreenHelper, GetServerListQueryMessage),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "array<spades::MainScreenServerItem@>@ GetServerList(string, bool)",
												  asMETHOD(gui::MainScreenHelper, GetServerList),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "string ConnectServer(string, int)",
												  asMETHOD(gui::MainScreenHelper, ConnectServer),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "string GetPendingErrorMessage()",
												  asMETHOD(gui::MainScreenHelper, GetPendingErrorMessage),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "string get_Credits()",
												  asMETHOD(gui::MainScreenHelper, GetCredits),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenHelper",
												  "PackageUpdateManager@ get_PackageUpdateManager()",
												  asMETHOD(gui::MainScreenHelper, GetPackageUpdateManager),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					
					r = eng->RegisterObjectBehaviour("MainScreenServerItem",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(gui::MainScreenServerItem, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("MainScreenServerItem",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(gui::MainScreenServerItem, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_Name()",
												  asMETHOD(gui::MainScreenServerItem, GetName),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_Address()",
												  asMETHOD(gui::MainScreenServerItem, GetAddress),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_MapName()",
												  asMETHOD(gui::MainScreenServerItem, GetMapName),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_GameMode()",
												  asMETHOD(gui::MainScreenServerItem, GetGameMode),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_Country()",
												  asMETHOD(gui::MainScreenServerItem, GetCountry),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "string get_Protocol()",
												  asMETHOD(gui::MainScreenServerItem, GetProtocol),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "int get_Ping()",
												  asMETHOD(gui::MainScreenServerItem, GetPing),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "int get_NumPlayers()",
												  asMETHOD(gui::MainScreenServerItem, GetNumPlayers),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "int get_MaxPlayers()",
												  asMETHOD(gui::MainScreenServerItem, GetMaxPlayers),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("MainScreenServerItem",
												  "bool get_Favorite()",
												  asMETHOD(gui::MainScreenServerItem, IsFavorite),
												  asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default:
					break;
			}
		}
	};
	
	static MainScreenHelperRegistrar registrar;
}

