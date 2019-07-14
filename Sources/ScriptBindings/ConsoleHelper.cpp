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
#include <Gui/ConsoleHelper.h>

namespace spades {
	class ConsoleHelperRegistrar : public ScriptObjectRegistrar {

	public:
		ConsoleHelperRegistrar() : ScriptObjectRegistrar("ConsoleHelper") {}

		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch (phase) {
				case PhaseObjectType:
					r = eng->RegisterObjectType("ConsoleHelper", 0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("ConsoleHelper", asBEHAVE_ADDREF, "void f()",
					                                 asMETHOD(gui::ConsoleHelper, AddRef),
					                                 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("ConsoleHelper", asBEHAVE_RELEASE, "void f()",
					                                 asMETHOD(gui::ConsoleHelper, Release),
					                                 asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(
					  "ConsoleHelper", "void ExecCommand(const string& in)",
					  asMETHOD(gui::ConsoleHelper, ExecCommand), asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default: break;
			}
		}
	};

	static ConsoleHelperRegistrar registrar;
} // namespace spades
