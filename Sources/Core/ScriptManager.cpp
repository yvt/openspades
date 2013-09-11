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
#include "Debug.h"
#include <vector>
#include "Exception.h"

namespace spades {
	
	ScriptManager *ScriptManager::GetInstance() {
		ScriptManager *m = new ScriptManager();
		return m;
	}
	
	static void MessageCallback(const asSMessageInfo *msg, void *param){
		const char *type = "ERR ";
		if( msg->type == asMSGTYPE_WARNING )
			type = "WARN";
		else if( msg->type == asMSGTYPE_INFORMATION )
			type = "INFO";
		SPLog("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
	}
	
	ScriptManager::ScriptManager() {
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		
		try{
			engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
			RegisterScriptAny(engine);
			RegisterScriptWeakRef(engine);
			RegisterScriptHandle(engine);
			RegisterScriptMath(engine);
			RegisterScriptMathComplex(engine);
			RegisterScriptArray(engine, true);
			RegisterStdString(engine);
			RegisterStdStringUtils(engine);
			RegisterScriptDictionary(engine);
			
			ScriptObjectRegistrar::RegisterAll(this,
											  ScriptObjectRegistrar::PhaseObjectType);
			ScriptObjectRegistrar::RegisterAll(this,
											  ScriptObjectRegistrar::PhaseObjectMember);
		}catch(...){
			engine->Release();
			throw;
		}
	}
	
	static std::string ASErrorToString(int ret){
		switch(ret){
			case asSUCCESS:
				return "Success";
			case asERROR:
				return "Unknown script engine error";
			case asCONTEXT_NOT_FINISHED:
				return "Context not finished";
			case asCONTEXT_NOT_PREPARED:
				return "Context not prepared";
			case asINVALID_ARG:
				return "Invalid argument";
			case asNO_FUNCTION:
				return "No function";
			case asNOT_SUPPORTED:
				return "Not supported";
			case asINVALID_NAME:
				return "Invalid name";
			case asNAME_TAKEN:
				return "Name taken";
			case asINVALID_DECLARATION:
				return "Invalid declaration";
			case asINVALID_OBJECT:
				return "Invalid object";
			case asINVALID_TYPE:
				return "Invalid type";
			case asALREADY_REGISTERED:
				return "Already registered";
			case asMULTIPLE_FUNCTIONS:
				return "Multiple functions";
			case asNO_MODULE:
				return "No module";
			case asNO_GLOBAL_VAR:
				return "No global variable";
			case asINVALID_CONFIGURATION:
				return "Invalid configuration";
			case asINVALID_INTERFACE:
				return "Invalid interface";
			case asCANT_BIND_ALL_FUNCTIONS:
				return "Failed to bind all functions";
			case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
				return "A lower array dimension is not registered";
			case asWRONG_CONFIG_GROUP:
				return "Wrong config group";
			case asCONFIG_GROUP_IS_IN_USE:
				return "Config group is in use";
			case asILLEGAL_BEHAVIOUR_FOR_TYPE:
				return "Illegal behaviour for the given type";
			case asWRONG_CALLING_CONV:
				return "Wrong call convention";
			case asBUILD_IN_PROGRESS:
				return "Build in progress";
			case asINIT_GLOBAL_VARS_FAILED:
				return "Failed to initialize global variables";
			case asOUT_OF_MEMORY:
				return "Out of memory";
			default:
				return "Unknown error code";
		}
	}
	
	void ScriptManager::CheckError(int ret){
		if(ret < 0){
			SPRaise("%s", ASErrorToString(ret).c_str());
		}
	}
	
	ScriptManager::~ScriptManager() {
		engine->Release();
	}
	
	static std::map<std::string, ScriptObjectRegistrar *> * registrars = NULL;
	
	ScriptObjectRegistrar::ScriptObjectRegistrar(const std::string& name):
	name(name){
		if(!registrars)
			registrars = new std::map<std::string, ScriptObjectRegistrar *>();
		(*registrars)[name]=this;
		std::fill(phaseDone, phaseDone + PhaseCount, false);
	}
	
	void ScriptObjectRegistrar::RegisterOne(const std::string &name,
											ScriptManager *manager,
											Phase phase) {
		std::map<std::string, ScriptObjectRegistrar *>::iterator it;
		it = registrars->find(name);
		if(it == registrars->end()){
			SPLog("WARNING: ScriptObjectRegistrar with name '%s' not found", name.c_str());
		}else{
			ScriptObjectRegistrar *r = it->second;
			if(r->phaseDone[(int)phase])
				return;
			r->phaseDone[(int)phase] = true;
			r->Register(manager,phase);
		}
	}
	
	void ScriptObjectRegistrar::RegisterAll(ScriptManager *manager, Phase phase) {
		std::map<std::string, ScriptObjectRegistrar *>::iterator it;
		for(it = registrars->begin(); it!= registrars->end(); it++){
			ScriptObjectRegistrar *r = it->second;
			if(r->phaseDone[(int)phase])
				continue;
			r->phaseDone[(int)phase] = true;
			r->Register(manager,phase);
		}
	}
	
}
