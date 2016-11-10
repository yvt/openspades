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
#include "ScriptFunction.h"
#include <Core/Debug.h>

namespace spades {
	
	void ScriptFunction::Load(asIScriptEngine *eng) {
		if(eng != lastEngine) {
			func = NULL;
			lastEngine = eng;
		}
		if(func == NULL){
			asIScriptModule *module = eng->GetModule("Client");
			eng->SetDefaultNamespace("spades");
			module->SetDefaultNamespace("spades");
			if(type.empty()) {
				
				func = eng->GetGlobalFunctionByDecl(decl.c_str());
				if(!func){
					func = module->GetFunctionByDecl(decl.c_str());
				}
				if(!func){
					SPRaise("Script function with signature '%s' not found.", decl.c_str());
				}
			}else{
				asITypeInfo *typ = eng->GetTypeInfoByName(type.c_str());
				if(!typ){
					typ = module->GetTypeInfoByName(type.c_str());
				}
				if(!typ){
					SPRaise("Script type '%s' not found.", type.c_str());
				}
				func = typ->GetMethodByDecl(decl.c_str());
				if(!func){
					SPRaise("Script method of '%s' with signature '%s' not found.",
							type.c_str(), decl.c_str());
				}
			}
		}
	}
	
	ScriptFunction::ScriptFunction(const std::string& decl):
	decl(decl), type(), lastEngine(NULL), func(NULL){}
	
	
	ScriptFunction::ScriptFunction(const std::string& type,
								   const std::string& decl):
	decl(decl), type(type), lastEngine(NULL), func(NULL){}
	
	ScriptContextHandle ScriptFunction::Prepare() {
		ScriptContextHandle ctx = ScriptManager::GetInstance()->GetContext();
		Load(ctx.GetContext()->GetEngine());
		
		ctx->Prepare(func);
		return ctx;
	}
	
}

