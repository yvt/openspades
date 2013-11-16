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
#include "IBlockSkin.h"
#include <Core/Debug.h>

namespace spades{
	namespace client {
		ScriptIBlockSkin::ScriptIBlockSkin(asIScriptObject *obj):
		obj(obj){}
		
		void ScriptIBlockSkin::SetReadyState(float v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IBlockSkin",
									   "void set_ReadyState(float)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgFloat(0, v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		void ScriptIBlockSkin::SetBlockColor(Vector3 v) {
			SPADES_MARK_FUNCTION_DEBUG();
			static ScriptFunction func("IBlockSkin",
									   "void set_BlockColor(Vector3)");
			ScriptContextHandle ctx = func.Prepare();
			int r;
			r = ctx->SetObject((void *)obj);
			ScriptManager::CheckError(r);
			r = ctx->SetArgObject(0, &v);
			ScriptManager::CheckError(r);
			ctx.ExecuteChecked();
		}
		
		class IBlockSkinRegistrar: public ScriptObjectRegistrar {
		public:
			IBlockSkinRegistrar():
			ScriptObjectRegistrar("IBlockSkin"){
				
			}
			virtual void Register(ScriptManager *manager, Phase phase) {
				asIScriptEngine *eng = manager->GetEngine();
				int r;
				eng->SetDefaultNamespace("spades");
				switch(phase){
					case PhaseObjectType:
						r = eng->RegisterInterface("IBlockSkin");
						manager->CheckError(r);
						break;
					case PhaseObjectMember:
						r = eng->RegisterInterfaceMethod("IBlockSkin",
														 "void set_ReadyState(float)");
						manager->CheckError(r);
						r = eng->RegisterInterfaceMethod("IBlockSkin",
														 "void set_BlockColor(Vector3)");
						manager->CheckError(r);
						break;
					default:
						
						break;
				}
			}
		};
		
		static IBlockSkinRegistrar registrar;
	}
}

