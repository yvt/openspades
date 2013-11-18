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
#include "IToolSkin.h"
#include <Core/Debug.h>

namespace spades{
	namespace client {
		ScriptIToolSkin::ScriptIToolSkin(asIScriptObject *obj):
		obj(obj){}
		
		void ScriptIToolSkin::SetSprintState(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void set_SprintState(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIToolSkin::SetRaiseState(float v){
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void set_RaiseState(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		void ScriptIToolSkin::SetMuted(bool v){
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void set_IsMuted(bool)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgByte(0, v?1:0);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		void ScriptIToolSkin::SetTeamColor(Vector3 v){
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void set_TeamColor(Vector3)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgObject(0, &v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		void ScriptIToolSkin::Update(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void Update(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIToolSkin::AddToScene() {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IToolSkin",
									   "void AddToScene()");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		class IToolSkinRegistrar: public ScriptObjectRegistrar {
		public:
			IToolSkinRegistrar():
			ScriptObjectRegistrar("IToolSkin"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterInterface("IToolSkin");
						manager->CheckError(r);
						
						
						break;
					case PhaseObjectMember:
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void set_SprintState(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void set_RaiseState(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void set_TeamColor(Vector3)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void set_IsMuted(bool)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void Update(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IToolSkin",
														 "void AddToScene()");
						manager->CheckError(r);
						
						break;
					default:
						
						break;
				}
			}
		};
		
		static IToolSkinRegistrar registrar;
	}
}

