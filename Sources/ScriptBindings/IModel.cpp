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
#include <Client/IModel.h>

namespace spades{
	namespace client {
		
		
		class ModelRegistrar: public ScriptObjectRegistrar {
		public:
			ModelRegistrar():
			ScriptObjectRegistrar("Model"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterObjectType("Model",
													0, asOBJ_REF);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("Model",
														 asBEHAVE_ADDREF, "void f()",
														 asMETHOD(IModel, AddRef),
														 asCALL_THISCALL);
						manager->CheckError(r);
						r = eng->RegisterObjectBehaviour("Model",
														 asBEHAVE_RELEASE, "void f()",
														 asMETHOD(IModel, Release),
														 asCALL_THISCALL);
						manager->CheckError(r);
						
						break;
					case PhaseObjectMember:
						break;
					default:
						
						break;
				}
			}
		};
		
		static ModelRegistrar registrar;
	}
}