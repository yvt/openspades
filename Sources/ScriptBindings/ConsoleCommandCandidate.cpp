/*
 Copyright (c) 2019 yvt

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
#include <Gui/ConsoleCommandCandidate.h>

namespace spades {
	class ConsoleCommandCandidateRegistrar : public ScriptObjectRegistrar {
		static void ConsoleCommandCandidateFactory1(gui::ConsoleCommandCandidate *p) {
			new (p) gui::ConsoleCommandCandidate();
		}
		static void ConsoleCommandCandidateFactory2(const gui::ConsoleCommandCandidate &other,
		                                            gui::ConsoleCommandCandidate *p) {
			new (p) gui::ConsoleCommandCandidate(other);
		}
		static void ConsoleCommandCandidateAssign(const gui::ConsoleCommandCandidate &other,
		                                          gui::ConsoleCommandCandidate *p) {
			*p = other;
		}
		static void ConsoleCommandCandidateDestructor(gui::ConsoleCommandCandidate *p) {
			p->~ConsoleCommandCandidate();
		}

	public:
		ConsoleCommandCandidateRegistrar() : ScriptObjectRegistrar("ConsoleCommandCandidate") {}

		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch (phase) {
				case PhaseObjectType:
					r = eng->RegisterObjectType("ConsoleCommandCandidate",
					                            sizeof(gui::ConsoleCommandCandidate),
					                            asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
					manager->CheckError(r);

					r = eng->RegisterObjectType("ConsoleCommandCandidateIterator", 0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour(
					  "ConsoleCommandCandidate", asBEHAVE_CONSTRUCT, "void f()",
					  asFUNCTION(ConsoleCommandCandidateFactory1), asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("ConsoleCommandCandidate", asBEHAVE_CONSTRUCT,
					                                 "void f(const ConsoleCommandCandidate& in)",
					                                 asFUNCTION(ConsoleCommandCandidateFactory2),
					                                 asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(
					  "ConsoleCommandCandidate", asBEHAVE_DESTRUCT, "void f()",
					  asFUNCTION(ConsoleCommandCandidateDestructor), asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod(
					  "ConsoleCommandCandidate", "void opAssign(const ConsoleCommandCandidate& in)",
					  asFUNCTION(ConsoleCommandCandidateAssign), asCALL_CDECL_OBJLAST);
					manager->CheckError(r);
					r = eng->RegisterObjectProperty("ConsoleCommandCandidate", "string Name",
					                                asOFFSET(gui::ConsoleCommandCandidate, name));
					manager->CheckError(r);
					r = eng->RegisterObjectProperty(
					  "ConsoleCommandCandidate", "string Description",
					  asOFFSET(gui::ConsoleCommandCandidate, description));
					manager->CheckError(r);

					r = eng->RegisterObjectBehaviour(
					  "ConsoleCommandCandidateIterator", asBEHAVE_ADDREF, "void f()",
					  asMETHOD(gui::ConsoleCommandCandidateIterator, AddRef), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour(
					  "ConsoleCommandCandidateIterator", asBEHAVE_RELEASE, "void f()",
					  asMETHOD(gui::ConsoleCommandCandidateIterator, Release), asCALL_THISCALL);
					manager->CheckError(r);

					r = eng->RegisterObjectMethod(
					  "ConsoleCommandCandidateIterator", "bool MoveNext()",
					  asMETHOD(gui::ConsoleCommandCandidateIterator, MoveNext), asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod(
					  "ConsoleCommandCandidateIterator",
					  "const ConsoleCommandCandidate &get_Current()",
					  asMETHOD(gui::ConsoleCommandCandidateIterator, GetCurrent), asCALL_THISCALL);
					manager->CheckError(r);
					break;
				default: break;
			}
		}
	};

	static ConsoleCommandCandidateRegistrar registrar;
} // namespace spades
