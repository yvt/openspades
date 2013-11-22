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
#include <Client/IFont.h>

namespace spades {
	class FontRegistrar: public ScriptObjectRegistrar {
		
		
	public:
		FontRegistrar():
		ScriptObjectRegistrar("Font") {}
		
		virtual void Register(ScriptManager *manager, Phase phase) {
			asIScriptEngine *eng = manager->GetEngine();
			int r;
			eng->SetDefaultNamespace("spades");
			switch(phase){
				case PhaseObjectType:
					r = eng->RegisterObjectType("Font",
												0, asOBJ_REF);
					manager->CheckError(r);
					break;
				case PhaseObjectMember:
					r = eng->RegisterObjectBehaviour("Font",
													 asBEHAVE_ADDREF,
													 "void f()",
													 asMETHOD(client::IFont, AddRef),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectBehaviour("Font",
													 asBEHAVE_RELEASE,
													 "void f()",
													 asMETHOD(client::IFont, Release),
													 asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Font",
												  "Vector2 Measure(const string& in)",
												  asMETHOD(client::IFont, Measure),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Font",
												  "void Draw(const string& in, Vector2, float, Vector4)",
												  asMETHOD(client::IFont, Draw),
												  asCALL_THISCALL);
					manager->CheckError(r);
					r = eng->RegisterObjectMethod("Font",
												  "void DrawShadow(const string& in, const Vector2& in, float, const Vector4& in, const Vector4& in)",
												  asMETHOD(client::IFont, DrawShadow),
												  asCALL_THISCALL);
					manager->CheckError(r);
					
					break;
				default:
					break;
			}
		}
	};
	
	static FontRegistrar registrar;
}

