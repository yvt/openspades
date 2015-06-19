/*
 Copyright (c) 2013 OpenSpades Developers
 
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
#include <Gui/StartupScreenHelper.h>

namespace spades {
	class StartupScreenHelperRegistrar: public ScriptObjectRegistrar {
		
		
	public:
		StartupScreenHelperRegistrar():
		ScriptObjectRegistrar("StartupScreenHelper") {}
		
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("StartupScreenHelper",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("StartupScreenHelper",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(gui::StartupScreenHelper, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("StartupScreenHelper",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(gui::StartupScreenHelper, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "void Start()",
												  asMETHOD(gui::StartupScreenHelper, Start),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "int GetNumVideoModes()",
												  asMETHOD(gui::StartupScreenHelper, GetNumVideoModes),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "int GetVideoModeWidth(int)",
												  asMETHOD(gui::StartupScreenHelper, GetVideoModeWidth),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "int GetVideoModeHeight(int)",
												  asMETHOD(gui::StartupScreenHelper, GetVideoModeHeight),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "int GetNumReportLines()",
												  asMETHOD(gui::StartupScreenHelper, GetNumReportLines),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "string GetReport()",
												  asMETHOD(gui::StartupScreenHelper, GetReport),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "string GetReportLineText(int)",
												  asMETHOD(gui::StartupScreenHelper, GetReportLineText),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "Vector4 GetReportLineColor(int)",
												  asMETHOD(gui::StartupScreenHelper, GetReportLineColor),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("StartupScreenHelper",
												  "string CheckConfigCapability(const string&in, const string&in)",
												  asMETHOD(gui::StartupScreenHelper, CheckConfigCapability),
												  asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default:
					break;
			}
		}
	};
	
	static StartupScreenHelperRegistrar registrar;
}

