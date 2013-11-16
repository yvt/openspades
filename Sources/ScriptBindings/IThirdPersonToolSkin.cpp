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
#include "IThirdPersonToolSkin.h"
#include <Core/Debug.h>

namespace spades{
	namespace client {
		ScriptIThirdPersonToolSkin::ScriptIThirdPersonToolSkin(asIScriptObject *obj):
		obj(obj){}
		
		void ScriptIThirdPersonToolSkin::SetOriginMatrix(Matrix4 m) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IThirdPersonToolSkin",
									   "void set_OriginMatrix(Matrix4)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgObject(0, &m);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		float ScriptIThirdPersonToolSkin::GetPitchBias() {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IThirdPersonToolSkin",
									   "float get_PitchBias()");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
			return ctx->GetReturnFloat();
		}
		
		class IThirdPersonToolSkinRegistrar: public ScriptObjectRegistrar {
		public:
			IThirdPersonToolSkinRegistrar():
			ScriptObjectRegistrar("IThirdPersonToolSkin"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterInterface("IThirdPersonToolSkin");
						manager->CheckError(r);
						
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterInterfaceMethod("IThirdPersonToolSkin",
														 "void set_OriginMatrix(Matrix4)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IThirdPersonToolSkin",
														 "float get_PitchBias()");
						manager->CheckError(r);
						break;
					default:
						
						break;
				}
			}
		};
		
		static IThirdPersonToolSkinRegistrar registrar;
	}
}

