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
#include "ISpadeSkin.h"
#include <Core/Debug.h>

namespace spades{
	namespace client {
		ScriptISpadeSkin::ScriptISpadeSkin(asIScriptObject *obj):
		obj(obj){}
		
		void ScriptISpadeSkin::SetActionType(SpadeActionType act) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("ISpadeSkin",
									   "void set_ActionType(SpadeActionType)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgDWord(0, (asDWORD)act);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptISpadeSkin::SetActionProgress(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("ISpadeSkin",
									   "void set_ActionProgress(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		class ISpadeSkinRegistrar: public ScriptObjectRegistrar {
		public:
			ISpadeSkinRegistrar():
			ScriptObjectRegistrar("ISpadeSkin"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterInterface("ISpadeSkin");
						manager->CheckError(r);
						
						r = eng->RegisterEnum("SpadeActionType");
						manager->CheckError(r);
						
						r = eng->RegisterEnumValue("SpadeActionType", "Idle", SpadeActionTypeIdle);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("SpadeActionType", "Bash", SpadeActionTypeBash);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("SpadeActionType", "Dig", SpadeActionTypeDig);
						manager->CheckError(r);
						r = eng->RegisterEnumValue("SpadeActionType", "DigStart", SpadeActionTypeDigStart);
						manager->CheckError(r);
						break;
					case PhaseObjectMember:
						r = eng->RegisterInterfaceMethod("ISpadeSkin",
														 "void set_ActionType(SpadeActionType)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("ISpadeSkin",
														 "void set_ActionProgress(float)");
						manager->CheckError(r);
						break;
					default:
						
						break;
				}
			}
		};
		
		static ISpadeSkinRegistrar registrar;
	}
}

